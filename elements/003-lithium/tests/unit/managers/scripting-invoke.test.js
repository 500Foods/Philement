import { describe, it, expect, vi } from 'vitest';
import {
  POLL_DELAYS_MS,
  nextPollDelay,
  isRunEnabled,
  clampRunTimeout,
  parseRunParams,
  buildRunPayload,
  formatRunFailure,
  formatJobEnvelope,
  pollScriptJob,
  createScriptInvoke,
} from '../../../src/managers/scripting/scripting-invoke.js';

const echo = { group_name: 'Api', script_name: 'Echo', invokable: 1 };

function runPane() {
  const root = document.createElement('div');
  root.innerHTML = `
    <div id="scripting-tab-run">
      <input type="checkbox" id="scripting-run-wait" checked>
      <input type="number" id="scripting-run-timeout" value="15">
      <button type="button" id="scripting-run-btn" disabled>Run</button>
      <pre id="scripting-run-result"></pre>
    </div>
  `;
  document.body.appendChild(root);
  return root;
}

function invokePanel(root, { invokeScript, getScriptJob, sleep, row = echo } = {}) {
  const api = {};
  const manager = { container: root, currentScript: row, app: { api } };
  const panel = createScriptInvoke(manager, {
    invokeScript,
    getScriptJob,
    sleep: sleep || (async () => {}),
  });
  panel.mount();
  return { panel, manager, api };
}

describe('nextPollDelay', () => {
  it('uses 250, 500, 1000, then 2000', () => {
    expect(POLL_DELAYS_MS).toEqual([250, 500, 1000, 2000]);
    expect([0, 1, 2, 3, 4].map(nextPollDelay)).toEqual([250, 500, 1000, 2000, 2000]);
  });
});

describe('isRunEnabled', () => {
  it('enables only an invokable Group.Name row', () => {
    expect(isRunEnabled(echo)).toBe(true);
    expect(isRunEnabled({ ...echo, invokable: '1' })).toBe(true);
    expect(isRunEnabled({ ...echo, invokable: 0 })).toBe(false);
    expect(isRunEnabled({ ...echo, invokable: null })).toBe(false);
    expect(isRunEnabled({ group_name: 'Api', invokable: 1 })).toBe(false);
  });
});

describe('buildRunPayload', () => {
  it('sends Group.Name, wait, and a clamped timeout', () => {
    expect(buildRunPayload({
      row: echo,
      paramsText: '{"hello":"world"}',
      wait: false,
      timeoutSeconds: 15,
    })).toEqual({
      script: 'Api.Echo',
      params: { hello: 'world' },
      wait: false,
      timeout_seconds: 15,
      stripped: false,
    });
    expect(clampRunTimeout(0)).toBe(1);
    expect(clampRunTimeout(90)).toBe(60);
    expect(clampRunTimeout('')).toBe(15);
    expect(buildRunPayload({
      row: echo,
      paramsText: '{}',
      wait: true,
      timeoutSeconds: 90,
    }).timeout_seconds).toBe(60);
  });

  it('strips params._hydrogen and rejects a non-object', () => {
    const built = buildRunPayload({
      row: echo,
      paramsText: '{"hello":1,"_hydrogen":{"sub":"x"}}',
      wait: true,
      timeoutSeconds: 15,
    });
    expect(built.stripped).toBe(true);
    expect(built.params).toEqual({ hello: 1 });
    expect(built.params._hydrogen).toBeUndefined();
    expect(parseRunParams('[]')).toEqual({ ok: false, message: 'Params must be a JSON object' });
    expect(parseRunParams('')).toEqual({ ok: true, params: {}, stripped: false });
    expect(() => buildRunPayload({
      row: echo,
      paramsText: 'nope',
      wait: true,
      timeoutSeconds: 15,
    })).toThrow('Params must be a JSON object');
  });

  it('does not add a run-as-role field', () => {
    const built = buildRunPayload({
      row: echo,
      paramsText: '{}',
      wait: true,
      timeoutSeconds: 15,
    });
    expect(Object.keys(built).sort()).toEqual(['params', 'script', 'stripped', 'timeout_seconds', 'wait']);
  });
});

describe('formatRunFailure', () => {
  it('calls script_not_found not runnable', () => {
    const error = {
      status: 404,
      data: { error: 'script_not_found' },
      serverError: { error: 'script_not_found', message: 'Script not found' },
    };
    const text = formatRunFailure('Api.Echo', error);
    expect(text).toContain('status: not_runnable');
    expect(text).toContain('Not runnable');
    expect(text).not.toContain('row missing');
    expect(text).not.toContain('Script not found');
  });
});

describe('pollScriptJob', () => {
  it('backs off until a terminal status', async () => {
    const statuses = ['pending', 'running', 'pending', 'pending', 'timeout'];
    const delays = [];
    let index = 0;
    const result = await pollScriptJob({
      jobId: 'ABCDE',
      getJob: async () => ({ status: statuses[index++], job_id: 'ABCDE' }),
      sleep: async (ms) => { delays.push(ms); },
    });
    expect(delays).toEqual([250, 500, 1000, 2000, 2000]);
    expect(result.stopped).toBe(false);
    expect(result.envelope.status).toBe('timeout');
  });

  it('stops on completed, failed, and killed', async () => {
    for (const status of ['completed', 'failed', 'killed']) {
      const result = await pollScriptJob({
        jobId: 'ABCDE',
        getJob: async () => ({ status, job_id: 'ABCDE', result: { ok: true } }),
        sleep: async () => {},
      });
      expect(result.envelope.status).toBe(status);
    }
  });

  it('stops on GET 404 and does not keep polling', async () => {
    const getJob = vi.fn(async () => {
      const error = new Error('missing');
      error.status = 404;
      error.serverError = { error: 'job_not_found', message: 'Job not found' };
      throw error;
    });
    const result = await pollScriptJob({
      jobId: 'ABCDE',
      getJob,
      sleep: async () => {},
    });
    expect(getJob).toHaveBeenCalledTimes(1);
    expect(result.missing).toBe(true);
    expect(result.notRunnable).toBe(false);
  });

  it('does not poll after cancellation', async () => {
    const getJob = vi.fn();
    const sleep = vi.fn();
    const result = await pollScriptJob({
      jobId: 'ABCDE',
      getJob,
      sleep,
      isCancelled: () => true,
    });
    expect(result.stopped).toBe(true);
    expect(sleep).not.toHaveBeenCalled();
    expect(getJob).not.toHaveBeenCalled();
  });

  it('stops when the row changes during the wait', async () => {
    let cancel = false;
    const getJob = vi.fn();
    const result = await pollScriptJob({
      jobId: 'ABCDE',
      getJob,
      sleep: async () => { cancel = true; },
      isCancelled: () => cancel,
    });
    expect(result.stopped).toBe(true);
    expect(getJob).not.toHaveBeenCalled();
  });
});

describe('createScriptInvoke', () => {
  it('keeps Run disabled unless the row is invokable', async () => {
    const root = runPane();
    const invokeScript = vi.fn();
    const { panel } = invokePanel(root, {
      invokeScript,
      row: { ...echo, invokable: 0 },
    });
    const button = root.querySelector('#scripting-run-btn');
    expect(button.disabled).toBe(true);
    await panel.run();
    expect(invokeScript).not.toHaveBeenCalled();
    panel.manager.currentScript = echo;
    panel.sync();
    expect(button.disabled).toBe(false);
    root.remove();
  });

  it('runs Api.Echo with wait true and paints the job id', async () => {
    const root = runPane();
    const invokeScript = vi.fn(async () => ({
      status: 'completed',
      job_id: 'ABCDE',
      result: { hello: 'world' },
      error: null,
    }));
    const { panel, api } = invokePanel(root, { invokeScript });
    panel.view = {
      state: { doc: { toString: () => '{"hello":"world","_hydrogen":{"sub":"x"}}' } },
      destroy() {},
    };
    await panel.run();
    expect(invokeScript).toHaveBeenCalledWith(api, {
      script: 'Api.Echo',
      params: { hello: 'world' },
      wait: true,
      timeoutSeconds: 15,
    });
    const text = root.querySelector('#scripting-run-result').textContent;
    expect(text).toContain('status: completed');
    expect(text).toContain('job_id: ABCDE');
    expect(text).toContain('hello');
    expect(root.querySelector('#scripting-tab-run').classList.contains('is-running')).toBe(false);
    root.remove();
  });

  it('polls wait false, shows the spinner, then the terminal status', async () => {
    const root = runPane();
    root.querySelector('#scripting-run-wait').checked = false;
    const pane = root.querySelector('#scripting-tab-run');
    const delays = [];
    let release;
    const getScriptJob = vi.fn(() => new Promise((resolve) => {
      release = resolve;
    }));
    const invokeScript = vi.fn(async () => ({
      status: 'pending',
      job_id: 'ABCDE',
      result: {},
      error: null,
    }));
    const { panel, api } = invokePanel(root, {
      invokeScript,
      getScriptJob,
      sleep: async (ms) => {
        delays.push(ms);
        expect(pane.classList.contains('is-running')).toBe(true);
      },
    });
    const pending = panel.run();
    await vi.waitFor(() => expect(getScriptJob).toHaveBeenCalledWith(api, 'ABCDE'));
    expect(delays).toEqual([250]);
    expect(pane.classList.contains('is-running')).toBe(true);
    release({
      status: 'completed',
      job_id: 'ABCDE',
      result: { hello: 'world' },
      error: null,
    });
    await pending;
    expect(pane.classList.contains('is-running')).toBe(false);
    expect(root.querySelector('#scripting-run-result').textContent).toContain('status: completed');
    expect(formatJobEnvelope('Api.Echo', { status: 'completed', job_id: 'ABCDE', result: { hello: 'world' } }))
      .toContain('job_id: ABCDE');
    root.remove();
  });

  it('drops a late poll when the row changes', async () => {
    const root = runPane();
    root.querySelector('#scripting-run-wait').checked = false;
    const getScriptJob = vi.fn();
    const invokeScript = vi.fn(async () => ({
      status: 'pending',
      job_id: 'ABCDE',
      result: {},
    }));
    const { panel } = invokePanel(root, {
      invokeScript,
      getScriptJob,
      sleep: async () => { panel.onRowChanged(); },
    });
    await panel.run();
    expect(getScriptJob).not.toHaveBeenCalled();
    expect(root.querySelector('#scripting-run-result').textContent).toBe('');
    root.remove();
  });

  it('paints not runnable for script_not_found and refuses bad JSON', async () => {
    const root = runPane();
    const error = new Error('HTTP 404');
    error.status = 404;
    error.serverError = { error: 'script_not_found', message: 'Script not found' };
    const invokeScript = vi.fn(async () => { throw error; });
    const { panel } = invokePanel(root, { invokeScript });
    await panel.run();
    const text = root.querySelector('#scripting-run-result').textContent;
    expect(text).toContain('Not runnable');
    expect(text).not.toContain('row missing');

    invokeScript.mockClear();
    panel.view = { state: { doc: { toString: () => '[' } }, destroy() {} };
    await panel.run();
    expect(invokeScript).not.toHaveBeenCalled();
    expect(root.querySelector('#scripting-run-result').textContent).toContain('Params must be a JSON object');
    root.remove();
  });

  it('clamps the timeout field', () => {
    const root = runPane();
    const { panel } = invokePanel(root, { invokeScript: vi.fn() });
    const input = root.querySelector('#scripting-run-timeout');
    input.value = '90';
    input.dispatchEvent(new window.Event('change'));
    expect(input.value).toBe('60');
    expect(panel.timeoutValue()).toBe(60);
    root.remove();
  });
});
