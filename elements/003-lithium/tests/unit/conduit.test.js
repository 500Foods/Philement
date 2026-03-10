/**
 * Conduit Shared Module – Unit Tests
 *
 * Tests the pure helper functions (buildQueryPayload, buildBatchPayload,
 * extractRows, extractBatchRows) and the thin API wrappers (authQuery,
 * authQueries, query, queries).
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import {
  buildQueryPayload,
  buildBatchPayload,
  extractRows,
  extractBatchRows,
  authQuery,
  authQueries,
  query,
  queries,
} from '../../src/shared/conduit.js';

// ─── Pure Functions ──────────────────────────────────────────────────────

describe('buildQueryPayload', () => {
  it('should build a payload with queryRef and empty params', () => {
    const payload = buildQueryPayload(25);
    expect(payload).toEqual({ query_ref: 25, params: {} });
  });

  it('should build a payload with typed params', () => {
    const payload = buildQueryPayload(32, { STRING: { SEARCH: 'USERS' } });
    expect(payload).toEqual({
      query_ref: 32,
      params: { STRING: { SEARCH: 'USERS' } },
    });
  });

  it('should build a payload with multiple param types', () => {
    const payload = buildQueryPayload(10, {
      STRING: { status: 'active' },
      INTEGER: { limit: 50 },
    });
    expect(payload).toEqual({
      query_ref: 10,
      params: {
        STRING: { status: 'active' },
        INTEGER: { limit: 50 },
      },
    });
  });

  it('should throw if queryRef is not a number', () => {
    expect(() => buildQueryPayload('25')).toThrow('queryRef must be a number');
    expect(() => buildQueryPayload(null)).toThrow('queryRef must be a number');
    expect(() => buildQueryPayload(undefined)).toThrow('queryRef must be a number');
  });

  it('should accept queryRef 0', () => {
    const payload = buildQueryPayload(0);
    expect(payload).toEqual({ query_ref: 0, params: {} });
  });

  it('should default params to empty object when omitted', () => {
    const payload = buildQueryPayload(1);
    expect(payload.params).toEqual({});
    expect(typeof payload.params).toBe('object');
  });
});

describe('buildBatchPayload', () => {
  it('should build a batch payload from query descriptors', () => {
    const payload = buildBatchPayload([
      { queryRef: 1, params: {} },
      { queryRef: 25 },
    ]);
    expect(payload).toEqual({
      queries: [
        { query_ref: 1, params: {} },
        { query_ref: 25, params: {} },
      ],
    });
  });

  it('should include database when provided', () => {
    const payload = buildBatchPayload(
      [{ queryRef: 1 }],
      'Acuranzo'
    );
    expect(payload).toEqual({
      database: 'Acuranzo',
      queries: [{ query_ref: 1, params: {} }],
    });
  });

  it('should not include database key when not provided', () => {
    const payload = buildBatchPayload([{ queryRef: 1 }]);
    expect(payload).not.toHaveProperty('database');
  });

  it('should pass through params for each query', () => {
    const payload = buildBatchPayload([
      { queryRef: 1, params: { STRING: { APIKEY: 'abc' } } },
      { queryRef: 25, params: { STRING: { status: 'active' } } },
    ]);
    expect(payload.queries[0].params).toEqual({ STRING: { APIKEY: 'abc' } });
    expect(payload.queries[1].params).toEqual({ STRING: { status: 'active' } });
  });

  it('should throw if queries is empty', () => {
    expect(() => buildBatchPayload([])).toThrow('non-empty array');
  });

  it('should throw if queries is not an array', () => {
    expect(() => buildBatchPayload('bad')).toThrow('non-empty array');
    expect(() => buildBatchPayload(null)).toThrow('non-empty array');
  });
});

describe('extractRows', () => {
  it('should extract from { data: [...] } shape', () => {
    const rows = extractRows({ data: [{ id: 1 }, { id: 2 }] });
    expect(rows).toEqual([{ id: 1 }, { id: 2 }]);
  });

  it('should extract from { rows: [...] } shape', () => {
    const rows = extractRows({ rows: [{ id: 1 }] });
    expect(rows).toEqual([{ id: 1 }]);
  });

  it('should extract from { results: [{ success: true, rows: [...] }] } shape', () => {
    const rows = extractRows({
      results: [{ success: true, rows: [{ id: 1 }] }],
    });
    expect(rows).toEqual([{ id: 1 }]);
  });

  it('should return the response itself if it is an array', () => {
    const rows = extractRows([{ id: 1 }, { id: 2 }]);
    expect(rows).toEqual([{ id: 1 }, { id: 2 }]);
  });

  it('should return empty array for null/undefined response', () => {
    expect(extractRows(null)).toEqual([]);
    expect(extractRows(undefined)).toEqual([]);
  });

  it('should return empty array for unrecognised shape', () => {
    expect(extractRows({ something: 'else' })).toEqual([]);
    expect(extractRows({})).toEqual([]);
  });

  it('should return empty array when results first entry is not successful', () => {
    const rows = extractRows({
      results: [{ success: false, error: 'SQL error' }],
    });
    expect(rows).toEqual([]);
  });

  it('should prefer data over rows when both exist', () => {
    const rows = extractRows({ data: [{ id: 'data' }], rows: [{ id: 'rows' }] });
    expect(rows).toEqual([{ id: 'data' }]);
  });
});

describe('extractBatchRows', () => {
  it('should return a Map keyed by query_ref', () => {
    const result = extractBatchRows({
      results: [
        { query_ref: 25, success: true, rows: [{ id: 1 }] },
        { query_ref: 32, success: true, rows: [{ id: 2 }] },
      ],
    });
    expect(result).toBeInstanceOf(Map);
    expect(result.get(25)).toEqual([{ id: 1 }]);
    expect(result.get(32)).toEqual([{ id: 2 }]);
  });

  it('should store error info for failed queries', () => {
    const result = extractBatchRows({
      results: [
        { query_ref: 25, success: false, error: 'SQL error' },
      ],
    });
    const entry = result.get(25);
    expect(entry).toHaveProperty('error', 'SQL error');
    expect(entry).toHaveProperty('rows');
    expect(entry.rows).toEqual([]);
  });

  it('should return empty Map for null/undefined', () => {
    expect(extractBatchRows(null)).toBeInstanceOf(Map);
    expect(extractBatchRows(null).size).toBe(0);
  });

  it('should return empty Map when results is not an array', () => {
    expect(extractBatchRows({ results: 'bad' }).size).toBe(0);
    expect(extractBatchRows({}).size).toBe(0);
  });

  it('should default rows to empty array for successful query with no rows', () => {
    const result = extractBatchRows({
      results: [{ query_ref: 1, success: true }],
    });
    expect(result.get(1)).toEqual([]);
  });
});


// ─── API Wrapper Functions ───────────────────────────────────────────────

describe('authQuery', () => {
  let mockApi;

  beforeEach(() => {
    mockApi = {
      post: vi.fn(),
    };
  });

  it('should call api.post with correct endpoint and payload', async () => {
    mockApi.post.mockResolvedValue({ data: [{ id: 1 }] });

    const rows = await authQuery(mockApi, 25);

    expect(mockApi.post).toHaveBeenCalledWith('conduit/auth_query', {
      query_ref: 25,
      params: {},
    });
    expect(rows).toEqual([{ id: 1 }]);
  });

  it('should pass params through to the payload', async () => {
    mockApi.post.mockResolvedValue({ data: [] });

    await authQuery(mockApi, 32, { STRING: { SEARCH: 'TEST' } });

    expect(mockApi.post).toHaveBeenCalledWith('conduit/auth_query', {
      query_ref: 32,
      params: { STRING: { SEARCH: 'TEST' } },
    });
  });

  it('should extract rows from the response', async () => {
    mockApi.post.mockResolvedValue({ data: [{ name: 'Query A' }, { name: 'Query B' }] });

    const rows = await authQuery(mockApi, 25);

    expect(rows).toHaveLength(2);
    expect(rows[0].name).toBe('Query A');
  });

  it('should propagate API errors', async () => {
    mockApi.post.mockRejectedValue(new Error('Network error'));

    await expect(authQuery(mockApi, 25)).rejects.toThrow('Network error');
  });
});

describe('authQueries', () => {
  let mockApi;

  beforeEach(() => {
    mockApi = {
      post: vi.fn(),
    };
  });

  it('should call api.post with auth_queries endpoint', async () => {
    mockApi.post.mockResolvedValue({
      results: [
        { query_ref: 25, success: true, rows: [{ id: 1 }] },
      ],
    });

    const result = await authQueries(mockApi, [{ queryRef: 25 }]);

    expect(mockApi.post).toHaveBeenCalledWith('conduit/auth_queries', {
      queries: [{ query_ref: 25, params: {} }],
    });
    expect(result.get(25)).toEqual([{ id: 1 }]);
  });

  it('should handle multiple queries', async () => {
    mockApi.post.mockResolvedValue({
      results: [
        { query_ref: 25, success: true, rows: [{ id: 1 }] },
        { query_ref: 32, success: true, rows: [{ id: 2 }] },
      ],
    });

    const result = await authQueries(mockApi, [
      { queryRef: 25 },
      { queryRef: 32, params: { STRING: { SEARCH: 'X' } } },
    ]);

    expect(result.size).toBe(2);
    expect(result.get(25)).toEqual([{ id: 1 }]);
    expect(result.get(32)).toEqual([{ id: 2 }]);
  });
});

describe('query', () => {
  let mockApi;

  beforeEach(() => {
    mockApi = {
      post: vi.fn(),
    };
  });

  it('should call api.post with query endpoint', async () => {
    mockApi.post.mockResolvedValue({ data: [{ id: 1 }] });

    const rows = await query(mockApi, 1);

    expect(mockApi.post).toHaveBeenCalledWith('conduit/query', {
      query_ref: 1,
      params: {},
    });
    expect(rows).toEqual([{ id: 1 }]);
  });

  it('should include database in payload when provided', async () => {
    mockApi.post.mockResolvedValue({ data: [] });

    await query(mockApi, 1, {}, 'Acuranzo');

    expect(mockApi.post).toHaveBeenCalledWith('conduit/query', {
      query_ref: 1,
      params: {},
      database: 'Acuranzo',
    });
  });
});

describe('queries', () => {
  let mockApi;

  beforeEach(() => {
    mockApi = {
      post: vi.fn(),
    };
  });

  it('should call api.post with queries endpoint', async () => {
    mockApi.post.mockResolvedValue({
      results: [{ query_ref: 1, success: true, rows: [] }],
    });

    await queries(mockApi, [{ queryRef: 1 }], 'Acuranzo');

    expect(mockApi.post).toHaveBeenCalledWith('conduit/queries', {
      database: 'Acuranzo',
      queries: [{ query_ref: 1, params: {} }],
    });
  });
});
