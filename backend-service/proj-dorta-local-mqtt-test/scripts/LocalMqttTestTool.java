import com.fasterxml.jackson.databind.ObjectMapper;
import org.dorta.iot.mqtt.protocol.IotMqttFrameBuilder;
import org.dorta.iot.mqtt.protocol.IotMqttFrameParser;
import org.dorta.iot.mqtt.protocol.IotMqttInboundMessage;
import org.dromara.mica.mqtt.codec.MqttQoS;
import org.dromara.mica.mqtt.codec.message.MqttPublishMessage;
import org.dromara.mica.mqtt.core.client.IMqttClientMessageListener;
import org.dromara.mica.mqtt.core.client.MqttClient;
import net.dreamlu.mica.net.core.ChannelContext;
import org.yaml.snakeyaml.LoaderOptions;
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.SafeConstructor;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.*;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/** Dorta 本地测试辅助工具。凭据只从当前本地配置读取，不写入输出或清单。 */
class LocalMqttTestTool {
    private static final ObjectMapper JSON = new ObjectMapper();
    private static final ZoneId ZONE = ZoneId.of("Asia/Shanghai");

    public static void main(String[] args) {
        try {
            run(args);
        } catch (Exception error) {
            // 不打印可能携带 JDBC URL、凭据或响应 Token 的原始异常消息。
            System.err.println("[local-test] FAILED: " + error.getClass().getSimpleName()
                + (error instanceof SQLException sql ? " SQLState=" + sql.getSQLState() : ""));
            System.exit(1);
        }
    }

    private static void run(String[] args) throws Exception {
        if (args.length != 11) throw new IllegalArgumentException("Use local-test.ps1");
        String action = args[0];
        if (action.equals("selftest")) { selfTest(); return; }
        String device = args[2];
        requireDevice(device);
        boolean writes = action.equals("seed") || action.equals("simulator");
        if (writes && !args[10].equals("allow-write")) throw new IllegalArgumentException("Explicit local write authorization required");
        Config config = new Config(Path.of(args[1]));
        Path runtime = Path.of(args[3]);
        Map<String, Object> snapshot;
        try (Connection db = config.connect(!action.equals("seed"))) {
            snapshot = device(db, device, action.equals("seed"));
            switch (action) {
                case "inspect" -> {
                    print(Map.of("environment", config.endpoints(), "device", snapshot));
                    db.rollback();
                }
                case "review" -> {
                    LocalDateTime since = args[9].equals("-") ? LocalDateTime.now(ZONE).minusHours(1) : LocalDateTime.parse(args[9]);
                    long id = Long.parseLong(snapshot.get("deviceId").toString());
                    List<Map<String, Object>> commands = query(db,
                        "SELECT seq_no,command_code,status,request_time,send_time,ack_time,ack_checksum_valid,"
                            + "CASE WHEN error_message IS NULL THEN 0 ELSE 1 END AS has_error"
                            + " FROM iot_device_command WHERE device_id=? AND request_time>=?"
                            + " AND command_code IN ('132','133') AND del_flag='0' ORDER BY request_time DESC LIMIT 20",
                        id, Timestamp.valueOf(since));
                    List<Map<String, Object>> rows = query(db,
                        "SELECT device_usage_id,seq_no,duration_minutes,start_time,used_count,surplus_count,del_flag"
                            + " FROM iot_device_usage WHERE device_id=? ORDER BY create_time DESC LIMIT 10", id);
                    print(Map.of("environment", config.endpoints(), "device", snapshot,
                        "since", since.toString(), "commands", commands, "recentUsageIncludingDeleted", rows));
                    db.rollback();
                }
                case "seed" -> seed(db, device, snapshot, Integer.parseInt(args[4]), runtime);
                case "simulator" -> db.rollback();
                default -> throw new IllegalArgumentException("Unknown action");
            }
        }
        // 读取初始状态后关闭连接，模拟期间不保持数据库会话。
        if (action.equals("simulator")) {
            simulate(config, device, snapshot, runtime, args[5], Integer.parseInt(args[6]),
                Integer.parseInt(args[7]), Integer.parseInt(args[8]));
        }
    }

    private static final class Config {
        private final Map<?, ?> master;
        private final Map<?, ?> mqtt;
        private final Map<?, ?> redis;
        private final String url;

        Config(Path file) throws Exception {
            Map<?, ?> root = new Yaml(new SafeConstructor(new LoaderOptions())).load(Files.readString(file));
            master = map(root, "spring", "datasource", "dynamic", "datasource", "master");
            mqtt = root.get("mqtt.client") instanceof Map<?, ?> dotted ? dotted : map(root, "mqtt", "client");
            redis = map(root, "spring", "data", "redis");
            url = text(master, "url");
            if (!localJdbc(url) || !text(mqtt, "ip").equals("127.0.0.1") || !text(mqtt, "port").equals("1884")
                || !Set.of("127.0.0.1", "localhost").contains(text(redis, "host"))) {
                throw new IllegalArgumentException("Only verified loopback Dorta test dependencies are supported");
            }
        }

        Connection connect(boolean readOnly) throws Exception {
            Class.forName("com.mysql.cj.jdbc.Driver");
            Properties properties = new Properties();
            properties.setProperty("user", text(master, "username"));
            properties.setProperty("password", text(master, "password"));
            properties.setProperty("connectTimeout", "5000");
            properties.setProperty("socketTimeout", "5000");
            Connection db = DriverManager.getConnection(url, properties);
            db.setReadOnly(readOnly);
            db.setAutoCommit(false);
            return db;
        }

        Map<String, Object> endpoints() {
            return Map.of("database", url.split("\\?", 2)[0], "mqtt", "127.0.0.1:1884",
                "redisHost", text(redis, "host"), "redisPort", text(redis, "port"), "redisDb", text(redis, "database"));
        }
    }

    private static boolean localJdbc(String url) {
        return url.matches("jdbc:mysql://(?:127\\.0\\.0\\.1|localhost):[0-9]+/dorta-center(?:\\?.*)?");
    }

    private static Map<?, ?> map(Map<?, ?> root, String... keys) {
        Map<?, ?> current = root;
        for (String key : keys) {
            if (!(current.get(key) instanceof Map<?, ?> next)) throw new IllegalArgumentException("Missing local configuration section");
            current = next;
        }
        return current;
    }

    private static String text(Map<?, ?> values, String key) {
        Object value = values.get(key);
        if (value == null) throw new IllegalArgumentException("Missing local configuration field");
        return value.toString();
    }

    private static void requireDevice(String device) {
        if (!device.matches("[A-Za-z0-9_-]{13}")) throw new IllegalArgumentException("Explicit 13-byte ASCII target required");
    }

    private static Map<String, Object> device(Connection db, String no, boolean lock) throws Exception {
        List<Map<String, Object>> identities = query(db,
            "SELECT device_id FROM iot_device WHERE device_no=? AND del_flag='0'" + (lock ? " FOR UPDATE" : ""), no);
        if (identities.size() != 1) throw new IllegalStateException("Expected exactly one target device");
        String id = identities.getFirst().get("device_id").toString();
        List<Map<String, Object>> states = query(db,
            "SELECT mqtt_status,used_count,surplus_count,last_online_time,last_heartbeat_time"
                + " FROM iot_device_info WHERE device_id=? AND del_flag='0'", Long.parseLong(id));
        if (states.size() != 1) throw new IllegalStateException("Expected one target device state");
        Map<String, Object> snapshot = new LinkedHashMap<>(states.getFirst());
        snapshot.put("deviceId", id);
        snapshot.put("deviceNo", no);
        snapshot.put("activeRecordCount", query(db,
            "SELECT COUNT(*) AS active_count FROM iot_device_usage WHERE device_id=? AND del_flag='0'",
            Long.parseLong(id)).getFirst().get("active_count"));
        return snapshot;
    }

    private static List<Map<String, Object>> query(Connection db, String sql, Object... params) throws Exception {
        try (PreparedStatement statement = db.prepareStatement(sql)) {
            statement.setQueryTimeout(5);
            for (int i = 0; i < params.length; i++) statement.setObject(i + 1, params[i]);
            try (ResultSet rows = statement.executeQuery()) {
                List<Map<String, Object>> result = new ArrayList<>();
                while (rows.next()) {
                    Map<String, Object> row = new LinkedHashMap<>();
                    for (int i = 1; i <= rows.getMetaData().getColumnCount(); i++) {
                        row.put(rows.getMetaData().getColumnLabel(i), rows.getString(i));
                    }
                    result.add(row);
                }
                return result;
            }
        }
    }

    private static int number(Map<String, Object> state, String key) {
        int value = Integer.parseInt(Objects.toString(state.get(key), "0"));
        if (value < 0 || value > 65535) throw new IllegalArgumentException("Counter outside protocol range");
        return value;
    }

    private static void seed(Connection db, String no, Map<String, Object> snapshot, int count, Path runtime) throws Exception {
        if (count < 1 || count > 10) throw new IllegalArgumentException("Seed count must be 1..10");
        long id = Long.parseLong(snapshot.get("deviceId").toString());
        int used = number(snapshot, "used_count");
        int surplus = number(snapshot, "surplus_count");
        int sequence = 1_000_000_000 + ThreadLocalRandom.current().nextInt(100_000_000);
        List<String> ids = new ArrayList<>();
        String sql = "INSERT INTO iot_device_usage (device_usage_id,device_id,seq_no,biz_id,duration_minutes,start_time,"
            + "surplus_count,used_count,create_time,update_time,del_flag)"
            + " SELECT UUID_SHORT(),d.device_id,?,?,?,?,?,?,NOW(),NOW(),'0' FROM iot_device d"
            + " WHERE d.device_id=? AND d.device_no=? AND d.del_flag='0'";
        try {
            for (int i = 0; i < count; i++) {
                if (!query(db, "SELECT device_usage_id FROM iot_device_usage WHERE device_id=? AND seq_no=?", id, sequence + i).isEmpty()) {
                    throw new IllegalStateException("Test sequence exists; no automatic retry");
                }
                try (PreparedStatement insert = db.prepareStatement(sql)) {
                    insert.setQueryTimeout(5);
                    insert.setInt(1, sequence + i);
                    insert.setInt(2, 1);
                    insert.setInt(3, 5 * (i + 1));
                    insert.setTimestamp(4, Timestamp.valueOf(LocalDateTime.now(ZONE).minusMinutes(15L * (count - i))));
                    insert.setInt(5, Math.min(65535, surplus + count - 1 - i));
                    insert.setInt(6, Math.max(used, count) - count + i + 1);
                    insert.setLong(7, id);
                    insert.setString(8, no);
                    if (insert.executeUpdate() != 1) throw new IllegalStateException("Target insert count mismatch");
                }
                ids.add(query(db, "SELECT device_usage_id FROM iot_device_usage WHERE device_id=? AND seq_no=?", id, sequence + i)
                    .getFirst().get("device_usage_id").toString());
            }
            db.commit();
        } catch (Exception error) { db.rollback(); throw error; }
        Map<String, Object> result = Map.of("deviceNo", no, "deviceId", Long.toString(id), "insertedCount", count,
            "recordIds", ids, "activeBefore", snapshot.get("activeRecordCount"), "deviceCountersModified", false);
        print(result);
        // 提交后清单保存失败只提示，不把已成功插入误报为回滚，避免重复造数。
        try { Files.writeString(runtime.resolve(no + ".last-seed.json"), JSON.writeValueAsString(result)); }
        catch (Exception error) { System.err.println("[local-test] Insert committed; manifest not saved. Preserve recordIds from output; do not retry blindly."); }
    }

    private static byte[] frame(long seq, int command, byte[] data) {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        bytes.write(0x5a); bytes.write(0xa5);
        for (int shift = 24; shift >= 0; shift -= 8) bytes.write((int) (seq >>> shift) & 255);
        bytes.write(data.length); bytes.write(command); bytes.writeBytes(data);
        int checksum = 0;
        for (byte b : bytes.toByteArray()) checksum = (checksum + (b & 255)) & 255;
        bytes.write(checksum);
        return bytes.toByteArray();
    }

    private static byte[] status(long seq, String no, int command, int used, int surplus) {
        ByteArrayOutputStream data = new ByteArrayOutputStream();
        data.writeBytes(no.getBytes(StandardCharsets.US_ASCII)); data.write(2);
        data.write(used >>> 8); data.write(used & 255); data.write(surplus >>> 8); data.write(surplus & 255);
        if (command == 0x88) data.writeBytes("                    ".getBytes(StandardCharsets.US_ASCII));
        return frame(seq, command, data.toByteArray());
    }

    private static void simulate(Config config, String no, Map<String, Object> snapshot, Path runtime, String mode,
                                 int seconds, int usedOverride, int surplusOverride) throws Exception {
        if (seconds < 1 || seconds > 86400 || !Set.of("success", "timeout").contains(mode)) throw new IllegalArgumentException("Invalid simulation mode or duration");
        AtomicInteger used = new AtomicInteger(usedOverride == -1 ? number(snapshot, "used_count") : usedOverride);
        int surplus = surplusOverride == -1 ? number(snapshot, "surplus_count") : surplusOverride;
        if (used.get() < 0 || used.get() > 65535 || surplus < 0 || surplus > 65535) throw new IllegalArgumentException("Counter outside protocol range");
        Path modeFile = runtime.resolve(no + ".mode");
        Files.writeString(modeFile, mode);
        AtomicLong sequence = new AtomicLong(900_000_001L);
        CountDownLatch subscribed = new CountDownLatch(1);
        MqttClient client = MqttClient.create().ip("127.0.0.1").port(1884)
            .clientId("dorta-local-sim-" + no).username(text(config.mqtt, "username")).password(text(config.mqtt, "password"))
            .reconnect(false).keepAliveSecs(30).shutdownTimeoutSec(2).connect();
        ScheduledExecutorService heartbeat = Executors.newSingleThreadScheduledExecutor();
        Thread shutdown = new Thread(() -> { heartbeat.shutdownNow(); client.stop(); });
        Runtime.getRuntime().addShutdownHook(shutdown);
        try {
            for (int i = 0; i < 30 && !client.isConnected(); i++) Thread.sleep(200);
            if (!client.isConnected()) throw new IllegalStateException("Local broker not connected");
            client.subQos0("device/" + no + "/ack", new IMqttClientMessageListener() {
                @Override public void onSubscribed(ChannelContext context, String topic, MqttQoS qos) { subscribed.countDown(); }
                @Override public void onMessage(ChannelContext context, String topic, MqttPublishMessage message, byte[] payload) {
                    try {
                        if (payload.length < 9 || ((payload[7] & 255) != 0x84 && (payload[7] & 255) != 0x85)) return;
                        IotMqttInboundMessage decoded = IotMqttFrameParser.parse(payload);
                        if (!no.equals(decoded.deviceNo())) return;
                        if (decoded.command() == 0x84) {
                            if (Files.readString(modeFile).trim().equals("timeout")) {
                                System.out.println("[SIM] RESET ignored for timeout device=" + no + " seq=" + decoded.seqNo());
                                return;
                            }
                            used.set(0);
                            publish(client, no, IotMqttFrameBuilder.ack(decoded.seqNo(), no, 0x84));
                            System.out.println("[SIM] RESET ACK device=" + no + " seq=" + decoded.seqNo() + " simulatedUsed=0");
                        } else {
                            publish(client, no, status(decoded.seqNo(), no, 0x85, used.get(), surplus));
                            System.out.println("[SIM] STATUS device=" + no + " used=" + used.get() + " surplus=" + surplus);
                        }
                    } catch (Exception error) { System.err.println("[SIM] Handler failed: " + error.getClass().getSimpleName()); }
                }
            });
            if (!subscribed.await(5, TimeUnit.SECONDS)) throw new IllegalStateException("Local topic subscription not acknowledged");
            publish(client, no, status(sequence.incrementAndGet(), no, 0x80, used.get(), surplus));
            heartbeat.scheduleAtFixedRate(() -> publish(client, no, status(sequence.incrementAndGet(), no, 0x88, used.get(), surplus)),
                30, 30, TimeUnit.SECONDS);
            System.out.println("[SIM] READY local-only device=" + no + " mode=" + mode + " durationSeconds=" + seconds);
            Thread.sleep(seconds * 1000L);
        } finally {
            heartbeat.shutdownNow(); client.stop(); Runtime.getRuntime().removeShutdownHook(shutdown);
        }
    }

    private static void publish(MqttClient client, String no, byte[] payload) {
        if (!client.publish("device/" + no + "/up", payload, false)) System.err.println("[SIM] Local publish failed");
    }

    private static void selfTest() throws Exception {
        String no = "SIM0000000001";
        requireDevice(no);
        if (!localJdbc("jdbc:mysql://127.0.0.1:3306/dorta-center?useSSL=false")
            || localJdbc("jdbc:mysql://remote.example:3306/dorta-center")
            || localJdbc("jdbc:mysql://127.0.0.1:3306/other-database")) throw new AssertionError("Local database restriction");
        for (int command : new int[]{0x80, 0x85, 0x88}) {
            IotMqttInboundMessage decoded = IotMqttFrameParser.parse(status(42, no, command, 20, 530));
            if (!no.equals(decoded.deviceNo()) || decoded.seqNo() != 42 || decoded.valueBo().getUsed() != 20
                || decoded.valueBo().getSurplus() != 530) throw new AssertionError("State frame compatibility");
        }
        byte[] ack = IotMqttFrameBuilder.ack(42L, no, 0x84);
        IotMqttInboundMessage decoded = IotMqttFrameParser.parse(ack);
        if (decoded.command() != 0x84 || decoded.seqNo() != 42 || !no.equals(decoded.deviceNo())) throw new AssertionError("ACK matching");
        ack[ack.length - 1] ^= 1;
        boolean rejected = false;
        try { IotMqttFrameParser.parse(ack); } catch (RuntimeException expected) { rejected = true; }
        if (!rejected) throw new AssertionError("Checksum rejection");
        print(Map.of("selfTest", "PASS", "databaseConnected", false, "mqttConnected", false,
            "checks", List.of("loopback database scope", "connection/status/heartbeat frames", "reset ACK identity", "checksum rejection")));
    }

    private static void print(Object value) throws Exception {
        System.out.println(JSON.writeValueAsString(value));
    }
}
