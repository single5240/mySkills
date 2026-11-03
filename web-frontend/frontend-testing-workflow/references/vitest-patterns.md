# Vitest 单元测试与 Mock 规范

本文档提供 Vue 3 + TypeScript + Vite 前端工程编写高质量 Vitest 单元测试的 5 类标准模式。以下示例中的模块与字段名仅作模式演示，落地时以目标工程真实类型与 API 契约为准。

## 1. 文件组织与命名约定

- **就近放置（Co-location）**：测试文件与被测模块放在同一目录，命名为 `<module>.test.ts`，例如：
  - `src/api/<module>/<business>/query.ts` -> `src/api/<module>/<business>/query.test.ts`
  - `src/hooks/async/useLoading.ts` -> `src/hooks/async/useLoading.test.ts`
  - `src/utils/<helper>.ts` -> `src/utils/<helper>.test.ts`
- **显式导入 Vitest API**：统一从 `'vitest'` 显式导入 `{ describe, expect, it, vi, beforeEach }`，不依赖全局注入，保证类型推导与 IDE 跳转稳定。
- **中文语义化用例名**：`describe` 写明被测业务模块或函数职责，`it` 写明“输入条件 + 预期行为/安全约束”，便于直接对照业务需求审查。

## 2. 模式一：查询参数清洗与不可变性测试

前端表单与日期选择器常产生空字符串 `''`、纯空格 `' '` 或 `null`。测试参数归一化函数时，必须同时断言**输出清洗正确**与**不污染原响应式查询对象**：

```ts
import { describe, expect, it } from 'vitest';
import { normalizeUsageQuery } from './query';

describe('查询参数清洗', () => {
  it('移除初始空日期与纯空格，保留有效筛选且不修改原始查询对象', () => {
    const query = {
      pageNum: 1,
      pageSize: 10,
      deviceNo: 'DEV001',
      params: { beginStartTime: '', endStartTime: ' ', beginCreateTime: null }
    };

    const normalized = normalizeUsageQuery(query);

    expect(normalized).toEqual({
      pageNum: 1,
      pageSize: 10,
      deviceNo: 'DEV001',
      params: {}
    });
    // 验证未直接篡改原对象的 params 引用（防止破坏页面 v-model 绑定）
    expect(query.params.beginStartTime).toBe('');
  });

  it('保留单端或非法日期，交由后端校验而非前端静默丢弃导致扩大操作范围', () => {
    expect(
      normalizeUsageQuery({
        pageNum: 1,
        pageSize: 10,
        params: { beginStartTime: 'invalid', endStartTime: '' }
      })
    ).toEqual({
      pageNum: 1,
      pageSize: 10,
      params: { beginStartTime: 'invalid' }
    });
  });

  it('允许未传查询参数', () => {
    expect(normalizeUsageQuery()).toBeUndefined();
  });
});
```

## 3. 模式二：高风险操作范围判定与二次确认文案测试

针对批量删除、全量清空、设备控制等高风险操作，将确认文案与触发条件抽离为纯函数，重点测试**防误操作边界**与**权限组合**：

- 无筛选条件但带分页参数（如 `pageNum: 3, pageSize: 10`）时，必须提示“删除全部、不受当前分页限制”。
- 仅单端填写时间范围时，显式展示“未填写 至 xxx”，不得误判为无筛选条件。
- 涉及联动副作用（如删除使用记录同时触发设备清零命令）时，分别测试：
  - 满足触发条件 + 有权限（`canResetUsed: true`）：提示将发送命令，且明确声明“命令发出不代表设备已执行，失败不会自动重发”；
  - 满足触发条件 + 无权限（`canResetUsed: false`）：提示无控制权限、仅执行主操作；
  - 带额外时间或序列号筛选：明确不触发清零副作用。

## 4. 模式三：返回状态与提示分流测试（外部副作用边界）

后端返回复合结果（如数据库操作成功，但异步通知或 MQTT 下发存在多种状态）时，测试所有枚举分支与兜底分支：

```ts
describe('操作结果提示分流', () => {
  it('命令已发送时提示去详情查看，不把已发送写成设备已执行成功', () => {
    const result = buildDeleteResultMessage({
      deletedCount: 5,
      resetStatus: 'SENT',
      deviceNo: 'DEV001'
    });
    expect(result.type).toBe('success');
    expect(result.message).toContain('已向设备 DEV001 发送清零已用次数命令');
  });

  it('命令发送失败时降级为 warning，并提示不会自动重发', () => {
    const result = buildDeleteResultMessage({
      deletedCount: 2,
      resetStatus: 'SEND_FAILED',
      deviceNo: 'DEV001',
      resetMessage: 'MQTT未连接'
    });
    expect(result.type).toBe('warning');
    expect(result.message).toContain('MQTT未连接');
    expect(result.message).toContain('不会自动重发');
  });
});
```

## 5. 模式四：Mock 请求封装层验证 API 契约

测试 `src/api/<module>/<business>/index.ts` 时，通过 `vi.mock('@/utils/request')` 拦截请求封装，验证传入的 `url`、`method`、`params`、`data` 及特殊 `headers`：

```ts
import { describe, expect, it, vi } from 'vitest';
import request from '@/utils/request';
import { delAllUsage } from './index';

vi.mock('@/utils/request', () => ({
  default: vi.fn(() => Promise.resolve({ data: {} }))
}));

describe('API 契约测试', () => {
  it('默认不传额外标志，且在发出请求前完成空参数清洗', async () => {
    await delAllUsage({
      pageNum: 1,
      pageSize: 10,
      deviceNo: 'DEV001',
      params: { beginStartTime: '', endStartTime: '' }
    });

    expect(request).toHaveBeenLastCalledWith({
      url: '/iot/deviceUsage/all',
      method: 'delete',
      params: {
        pageNum: 1,
        pageSize: 10,
        deviceNo: 'DEV001',
        params: {}
      }
    });
  });
});
```

## 6. 模式五：异步 Hooks、竞态与错误回滚测试

测试 `src/hooks/` 或页面异步状态控制器时，使用受控 Promise（Deferred 模式）验证时序与异常出口：

```ts
const createDeferred = <T>() => {
  let resolve!: (value: T) => void;
  let reject!: (reason?: unknown) => void;
  const promise = new Promise<T>((res, rej) => {
    resolve = res;
    reject = rej;
  });
  return { promise, resolve, reject };
};
```

重点覆盖以下场景：

1. **异常释放 Loading**：当传入 `withLoading` 的异步函数 `reject` 时，断言错误继续向上抛出（或按约定处理），且 `loading.value` 最终恢复为 `false`。
2. **并发竞态防护**：先后发起查询 A（慢）和查询 B（快），先 `resolve` B 再 `resolve` A，断言最终列表数据为 B 的结果，且 A 的完成不会提前关掉 B 的 `loading`。
3. **状态切换失败回滚**：模拟 `changeStatus` 接口 `reject`，断言行内状态值回滚为切换前的值。
