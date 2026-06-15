import { l as log, S as Status, a as Subsystems } from './index-CEuz7QhE.js';

/*!
 * OverlayScrollbars
 * Version: 2.16.0
 *
 * Copyright (c) Rene Haas | KingSora.
 * https://github.com/KingSora
 *
 * Released under the MIT license.
 */
const createCache = (t, n) => {
  const {o: o, i: s, u: e} = t;
  let c = o;
  let r;
  const cacheUpdateContextual = (t, n) => {
    const o = c;
    const i = t;
    const l = n || (s ? !s(o, i) : o !== i);
    if (l || e) {
      c = i;
      r = o;
    }
    return [ c, l, r ];
  };
  const cacheUpdateIsolated = t => cacheUpdateContextual(n(c, r), t);
  const getCurrentCache = t => [ c, !!t, r ];
  return [ n ? cacheUpdateIsolated : cacheUpdateContextual, getCurrentCache ];
};

const t = typeof window !== "undefined" && typeof HTMLElement !== "undefined" && !!window.document;

const n = t ? window : {};

const o = Math.max;

const s = Math.min;

const e = Math.round;

const c = Math.abs;

const r = Math.sign;

const i = n.cancelAnimationFrame;

const l = n.requestAnimationFrame;

const a = n.setTimeout;

const u = n.clearTimeout;

const getApi = t => typeof n[t] !== "undefined" ? n[t] : void 0;

const f = getApi("MutationObserver");

const _ = getApi("IntersectionObserver");

const d = getApi("ResizeObserver");

const p = getApi("ScrollTimeline");

const isUndefined = t => t === void 0;

const isNull = t => t === null;

const isNumber = t => typeof t === "number";

const isString = t => typeof t === "string";

const isBoolean = t => typeof t === "boolean";

const isFunction = t => typeof t === "function";

const isArray = t => Array.isArray(t);

const isObject = t => typeof t === "object" && !isArray(t) && !isNull(t);

const isArrayLike = t => {
  const n = !!t && t.length;
  const o = isNumber(n) && n > -1 && n % 1 == 0;
  return isArray(t) || !isFunction(t) && o ? n > 0 && isObject(t) ? n - 1 in t : true : false;
};

const isPlainObject = t => !!t && t.constructor === Object;

const isHTMLElement = t => t instanceof HTMLElement;

const isElement = t => t instanceof Element;

const animationCurrentTime = () => performance.now();

const animateNumber = (t, n, s, e, c) => {
  let r = 0;
  const a = animationCurrentTime();
  const u = o(0, s);
  const frame = s => {
    const i = animationCurrentTime();
    const f = i - a;
    const _ = f >= u;
    const d = s ? 1 : 1 - (o(0, a + u - i) / u || 0);
    const p = (n - t) * (isFunction(c) ? c(d, d * u, 0, 1, u) : d) + t;
    const v = _ || d === 1;
    e(p, d, v);
    r = v ? 0 : l((() => frame()));
  };
  frame();
  return t => {
    i(r);
    if (t) {
      frame(t);
    }
  };
};

function each(t, n) {
  if (isArrayLike(t)) {
    for (let o = 0; o < t.length; o++) {
      if (n(t[o], o, t) === false) {
        break;
      }
    }
  } else if (t) {
    each(Object.keys(t), (o => n(t[o], o, t)));
  }
  return t;
}

const inArray = (t, n) => t.indexOf(n) >= 0;

const concat = (t, n) => t.concat(n);

const push = (t, n, o) => {
  if (!isString(n) && isArrayLike(n)) {
    Array.prototype.push.apply(t, n);
  } else {
    t.push(n);
  }
  return t;
};

const from = t => Array.from(t || []);

const createOrKeepArray = t => {
  if (isArray(t)) {
    return t;
  }
  return !isString(t) && isArrayLike(t) ? from(t) : [ t ];
};

const isEmptyArray = t => !!t && !t.length;

const deduplicateArray = t => from(new Set(t));

const runEachAndClear = (t, n, o) => {
  const runFn = t => t ? t.apply(void 0, n || []) : true;
  each(t, runFn);
  if (!o) {
    t.length = 0;
  }
};

const v = "paddingTop";

const g = "paddingRight";

const h = "paddingLeft";

const b = "paddingBottom";

const y = "marginLeft";

const w = "marginRight";

const S = "marginBottom";

const m = "overflowX";

const O = "overflowY";

const C = "width";

const $ = "height";

const x = "visible";

const H = "hidden";

const E = "scroll";

const capitalizeFirstLetter = t => {
  const n = String(t || "");
  return n ? n[0].toUpperCase() + n.slice(1) : "";
};

const equal = (t, n, o, s) => {
  if (t && n) {
    let s = true;
    each(o, (o => {
      const e = t[o];
      const c = n[o];
      if (e !== c) {
        s = false;
      }
    }));
    return s;
  }
  return false;
};

const equalWH = (t, n) => equal(t, n, [ "w", "h" ]);

const equalXY = (t, n) => equal(t, n, [ "x", "y" ]);

const equalTRBL = (t, n) => equal(t, n, [ "t", "r", "b", "l" ]);

const bind = (t, ...n) => t.bind(0, ...n);

const selfClearTimeout = t => {
  let n;
  const o = t ? a : l;
  const s = t ? u : i;
  return [ e => {
    s(n);
    n = o((() => e()), isFunction(t) ? t() : t);
  }, () => s(n) ];
};

const getDebouncer = t => {
  const n = isFunction(t) ? t() : t;
  if (isNumber(n)) {
    const t = n ? a : l;
    const o = n ? u : i;
    return s => {
      const e = t((() => s()), n);
      return () => {
        o(e);
      };
    };
  }
  return n && n._;
};

const debounce = (t, n) => {
  const {p: o, v: s, S: e, m: c} = n || {};
  let r;
  let i;
  let l;
  let a;
  const u = function invokeFunctionToDebounce(n) {
    if (i) {
      i();
    }
    if (r) {
      r();
    }
    a = i = r = l = void 0;
    t.apply(this, n);
  };
  const mergeParms = t => c && l ? c(l, t) : t;
  const flush = () => {
    if (i && l) {
      u(mergeParms(l) || l);
    }
  };
  const f = function debouncedFn() {
    const t = from(arguments);
    const n = getDebouncer(o);
    if (n) {
      const o = typeof e === "function" ? e() : e;
      const c = getDebouncer(s);
      const f = mergeParms(t);
      const _ = f || t;
      const d = u.bind(0, _);
      if (i) {
        i();
      }
      if (o && !a) {
        d();
        a = true;
        i = n((() => a = void 0));
      } else {
        i = n(d);
        if (c && !r) {
          r = c(flush);
        }
      }
      l = _;
    } else {
      u(t);
    }
  };
  f.O = flush;
  return f;
};

const hasOwnProperty = (t, n) => Object.prototype.hasOwnProperty.call(t, n);

const keys = t => t ? Object.keys(t) : [];

const assignDeep = (t, n, o, s, e, c, r) => {
  const i = [ n, o, s, e, c, r ];
  if ((typeof t !== "object" || isNull(t)) && !isFunction(t)) {
    t = {};
  }
  each(i, (n => {
    each(n, ((o, s) => {
      const e = n[s];
      if (t === e) {
        return true;
      }
      const c = isArray(e);
      if (e && isPlainObject(e)) {
        const n = t[s];
        let o = n;
        if (c && !isArray(n)) {
          o = [];
        } else if (!c && !isPlainObject(n)) {
          o = {};
        }
        t[s] = assignDeep(o, e);
      } else {
        t[s] = c ? e.slice() : e;
      }
    }));
  }));
  return t;
};

const removeUndefinedProperties = (t, n) => each(assignDeep({}, t), ((t, n, o) => {
  if (t === void 0) {
    delete o[n];
  } else if (t && isPlainObject(t)) {
    o[n] = removeUndefinedProperties(t);
  }
}));

const isEmptyObject = t => !keys(t).length;

const noop = () => {};

const capNumber = (t, n, e) => o(t, s(n, e));

const getDomTokensArray = t => deduplicateArray((isArray(t) ? t : (t || "").split(" ")).filter((t => t)));

const getAttr = (t, n) => t && t.getAttribute(n);

const hasAttr = (t, n) => t && t.hasAttribute(n);

const setAttrs = (t, n, o) => {
  each(getDomTokensArray(n), (n => {
    if (t) {
      t.setAttribute(n, String(o || ""));
    }
  }));
};

const removeAttrs = (t, n) => {
  each(getDomTokensArray(n), (n => t && t.removeAttribute(n)));
};

const domTokenListAttr = (t, n) => {
  const o = getDomTokensArray(getAttr(t, n));
  const s = bind(setAttrs, t, n);
  const domTokenListOperation = (t, n) => {
    const s = new Set(o);
    each(getDomTokensArray(t), (t => {
      s[n](t);
    }));
    return from(s).join(" ");
  };
  return {
    C: t => s(domTokenListOperation(t, "delete")),
    $: t => s(domTokenListOperation(t, "add")),
    H: t => {
      const n = getDomTokensArray(t);
      return n.reduce(((t, n) => t && o.includes(n)), n.length > 0);
    }
  };
};

const removeAttrClass = (t, n, o) => {
  domTokenListAttr(t, n).C(o);
  return bind(addAttrClass, t, n, o);
};

const addAttrClass = (t, n, o) => {
  domTokenListAttr(t, n).$(o);
  return bind(removeAttrClass, t, n, o);
};

const addRemoveAttrClass = (t, n, o, s) => (s ? addAttrClass : removeAttrClass)(t, n, o);

const hasAttrClass = (t, n, o) => domTokenListAttr(t, n).H(o);

const createDomTokenListClass = t => domTokenListAttr(t, "class");

const removeClass = (t, n) => {
  createDomTokenListClass(t).C(n);
};

const addClass = (t, n) => {
  createDomTokenListClass(t).$(n);
  return bind(removeClass, t, n);
};

const find = (t, n) => {
  const o = n ? isElement(n) && n : document;
  return o ? from(o.querySelectorAll(t)) : [];
};

const findFirst = (t, n) => {
  const o = n ? isElement(n) && n : document;
  return o && o.querySelector(t);
};

const is = (t, n) => isElement(t) && t.matches(n);

const isBodyElement = t => is(t, "body");

const contents = t => t ? from(t.childNodes) : [];

const parent = t => t && t.parentElement;

const closest = (t, n) => isElement(t) && t.closest(n);

const getFocusedElement = t => document.activeElement;

const liesBetween = (t, n, o) => {
  const s = closest(t, n);
  const e = t && findFirst(o, s);
  const c = closest(e, n) === s;
  return s && e ? s === t || e === t || c && closest(closest(t, o), n) !== s : false;
};

const removeElements = t => {
  each(createOrKeepArray(t), (t => {
    const n = parent(t);
    if (t && n) {
      n.removeChild(t);
    }
  }));
};

const appendChildren = (t, n) => bind(removeElements, t && n && each(createOrKeepArray(n), (n => {
  if (n) {
    t.appendChild(n);
  }
})));

let D;

const getTrustedTypePolicy = () => D;

const setTrustedTypePolicy = t => {
  D = t;
};

const createDiv = t => {
  const n = document.createElement("div");
  setAttrs(n, "class", t);
  return n;
};

const createDOM = t => {
  const n = createDiv();
  const o = getTrustedTypePolicy();
  const s = t.trim();
  n.innerHTML = o ? o.createHTML(s) : s;
  return each(contents(n), (t => removeElements(t)));
};

const getCSSVal = (t, n) => t.getPropertyValue(n) || t[n] || "";

const validFiniteNumber = t => {
  const n = t || 0;
  return isFinite(n) ? n : 0;
};

const parseToZeroOrNumber = t => validFiniteNumber(parseFloat(t || ""));

const roundCssNumber = t => Math.round(t * 1e4) / 1e4;

const numberToCssPx = t => `${roundCssNumber(validFiniteNumber(t))}px`;

function setStyles(t, n) {
  t && n && each(n, ((n, o) => {
    try {
      const s = t.style;
      const e = isNull(n) || isBoolean(n) ? "" : isNumber(n) ? numberToCssPx(n) : n;
      if (o.indexOf("--") === 0) {
        s.setProperty(o, e);
      } else {
        s[o] = e;
      }
    } catch (s) {}
  }));
}

function getStyles(t, o, s) {
  const e = isString(o);
  let c = e ? "" : {};
  if (t) {
    const r = n.getComputedStyle(t, s) || t.style;
    c = e ? getCSSVal(r, o) : from(o).reduce(((t, n) => {
      t[n] = getCSSVal(r, n);
      return t;
    }), c);
  }
  return c;
}

const topRightBottomLeft = (t, n, o) => {
  const s = n ? `${n}-` : "";
  const e = o ? `-${o}` : "";
  const c = `${s}top${e}`;
  const r = `${s}right${e}`;
  const i = `${s}bottom${e}`;
  const l = `${s}left${e}`;
  const a = getStyles(t, [ c, r, i, l ]);
  return {
    t: parseToZeroOrNumber(a[c]),
    r: parseToZeroOrNumber(a[r]),
    b: parseToZeroOrNumber(a[i]),
    l: parseToZeroOrNumber(a[l])
  };
};

const getTrasformTranslateValue = (t, n) => `translate${isObject(t) ? `(${t.x},${t.y})` : `${n ? "X" : "Y"}(${t})`}`;

const elementHasDimensions = t => !!(t.offsetWidth || t.offsetHeight || t.getClientRects().length);

const z = {
  w: 0,
  h: 0
};

const getElmWidthHeightProperty = (t, n) => n ? {
  w: n[`${t}Width`],
  h: n[`${t}Height`]
} : z;

const getWindowSize = t => getElmWidthHeightProperty("inner", t || n);

const I = bind(getElmWidthHeightProperty, "offset");

const A = bind(getElmWidthHeightProperty, "client");

const T = bind(getElmWidthHeightProperty, "scroll");

const getFractionalSize = t => {
  const n = parseFloat(getStyles(t, C)) || 0;
  const o = parseFloat(getStyles(t, $)) || 0;
  return {
    w: n - e(n),
    h: o - e(o)
  };
};

const getBoundingClientRect = t => t.getBoundingClientRect();

const hasDimensions = t => !!t && elementHasDimensions(t);

const domRectHasDimensions = t => !!(t && (t[$] || t[C]));

const domRectAppeared = (t, n) => {
  const o = domRectHasDimensions(t);
  const s = domRectHasDimensions(n);
  return !s && o;
};

const removeEventListener = (t, n, o, s) => {
  each(getDomTokensArray(n), (n => {
    if (t) {
      t.removeEventListener(n, o, s);
    }
  }));
};

const addEventListener = (t, n, o, s) => {
  var e;
  const c = (e = s && s.D) != null ? e : true;
  const r = s && s.I || false;
  const i = s && s.A || false;
  const l = {
    passive: c,
    capture: r
  };
  return bind(runEachAndClear, getDomTokensArray(n).map((n => {
    const s = i ? e => {
      removeEventListener(t, n, s, r);
      if (o) {
        o(e);
      }
    } : o;
    if (t) {
      t.addEventListener(n, s, l);
    }
    return bind(removeEventListener, t, n, s, r);
  })));
};

const stopPropagation = t => t.stopPropagation();

const preventDefault = t => t.preventDefault();

const stopAndPrevent = t => stopPropagation(t) || preventDefault(t);

const scrollElementTo = (t, n) => {
  const {x: o, y: s} = isNumber(n) ? {
    x: n,
    y: n
  } : n || {};
  isNumber(o) && (t.scrollLeft = o);
  isNumber(s) && (t.scrollTop = s);
};

const getElementScroll = t => ({
  x: t.scrollLeft,
  y: t.scrollTop
});

const getZeroScrollCoordinates = () => ({
  T: {
    x: 0,
    y: 0
  },
  k: {
    x: 0,
    y: 0
  }
});

const sanitizeScrollCoordinates = (t, n) => {
  const {T: o, k: s} = t;
  const {w: e, h: i} = n;
  const sanitizeAxis = (t, n, o) => {
    let s = r(t) * o;
    let e = r(n) * o;
    if (s === e) {
      const o = c(t);
      const r = c(n);
      e = o > r ? 0 : e;
      s = o < r ? 0 : s;
    }
    s = s === e ? 0 : s;
    return [ s + 0, e + 0 ];
  };
  const [l, a] = sanitizeAxis(o.x, s.x, e);
  const [u, f] = sanitizeAxis(o.y, s.y, i);
  return {
    T: {
      x: l,
      y: u
    },
    k: {
      x: a,
      y: f
    }
  };
};

const isDefaultDirectionScrollCoordinates = ({T: t, k: n}) => {
  const getAxis = (t, n) => t === 0 && t <= n;
  return {
    x: getAxis(t.x, n.x),
    y: getAxis(t.y, n.y)
  };
};

const getScrollCoordinatesPercent = ({T: t, k: n}, o) => {
  const getAxis = (t, n, o) => capNumber(0, 1, (t - o) / (t - n) || 0);
  return {
    x: getAxis(t.x, n.x, o.x),
    y: getAxis(t.y, n.y, o.y)
  };
};

const focusElement = t => {
  if (t && t.focus) {
    t.focus({
      preventScroll: true,
      focusVisible: false
    });
  }
};

const manageListener = (t, n) => {
  each(createOrKeepArray(n), t);
};

const createEventListenerHub = t => {
  const n = new Map;
  const removeEvent = (t, o) => {
    if (t) {
      const s = n.get(t);
      manageListener((t => {
        if (s) {
          s[t ? "delete" : "clear"](t);
        }
      }), o);
    } else {
      n.forEach((t => {
        t.clear();
      }));
      n.clear();
    }
  };
  const addEvent = (t, o) => {
    if (isString(t)) {
      const s = n.get(t) || new Set;
      n.set(t, s);
      manageListener((t => {
        if (isFunction(t)) {
          s.add(t);
        }
      }), o);
      return bind(removeEvent, t, o);
    }
    if (isBoolean(o) && o) {
      removeEvent();
    }
    const s = keys(t);
    const e = [];
    each(s, (n => {
      const o = t[n];
      if (o) {
        push(e, addEvent(n, o));
      }
    }));
    return bind(runEachAndClear, e);
  };
  const triggerEvent = (t, o) => {
    each(from(n.get(t)), (t => {
      if (o && !isEmptyArray(o)) {
        t.apply(0, o);
      } else {
        t();
      }
    }));
  };
  addEvent(t || {});
  return [ addEvent, removeEvent, triggerEvent ];
};

const k = {};

const M = {};

const addPlugins = t => {
  each(t, (t => each(t, ((n, o) => {
    k[o] = t[o];
  }))));
};

const registerPluginModuleInstances = (t, n, o) => keys(t).map((s => {
  const {static: e, instance: c} = t[s];
  const [r, i, l] = o || [];
  const a = o ? c : e;
  if (a) {
    const t = o ? a(r, i, n) : a(n);
    return (l || M)[s] = t;
  }
}));

const getInstancePluginModuleInstance = (t, n) => t[n];

const getStaticPluginModuleInstance = t => getInstancePluginModuleInstance(M, t);

const R = "__osOptionsValidationPlugin";

const V = `data-overlayscrollbars`;

const L = "os-environment";

const P = `${L}-scrollbar-hidden`;

const U = `${V}-initialize`;

const N = "noClipping";

const q = `${V}-body`;

const B = V;

const F = "host";

const j = `${V}-viewport`;

const X = m;

const Y = O;

const W = "arrange";

const J = "measuring";

const G = "scrolling";

const K = "scrollbarHidden";

const Q = "noContent";

const Z = `${V}-padding`;

const tt = `${V}-content`;

const nt = "os-size-observer";

const ot = `${nt}-appear`;

const st = `${nt}-listener`;

const et = `${st}-scroll`;

const ct = `${st}-item`;

const rt = `${ct}-final`;

const it = "os-trinsic-observer";

const lt = "os-theme-none";

const at = "os-scrollbar";

const ut = `${at}-rtl`;

const ft = `${at}-horizontal`;

const _t = `${at}-vertical`;

const dt = `${at}-track`;

const pt = `${at}-handle`;

const vt = `${at}-visible`;

const gt = `${at}-cornerless`;

const ht = `${at}-interaction`;

const bt = `${at}-unusable`;

const yt = `${at}-auto-hide`;

const wt = `${yt}-hidden`;

const St = `${at}-wheel`;

const mt = `${dt}-interactive`;

const Ot = `${pt}-interactive`;

const Ct = "__osSizeObserverPlugin";

const $t = /* @__PURE__ */ (() => ({
  [Ct]: {
    static: () => (t, n, o) => {
      const s = 3333333;
      const e = "scroll";
      const c = createDOM(`<div class="${ct}" dir="ltr"><div class="${ct}"><div class="${rt}"></div></div><div class="${ct}"><div class="${rt}" style="width: 200%; height: 200%"></div></div></div>`);
      const r = c[0];
      const a = r.lastChild;
      const u = r.firstChild;
      const f = u == null ? void 0 : u.firstChild;
      let _ = I(r);
      let d = _;
      let p = false;
      let v;
      const reset = () => {
        scrollElementTo(u, s);
        scrollElementTo(a, s);
      };
      const onResized = t => {
        v = 0;
        if (p) {
          _ = d;
          n(t === true);
        }
      };
      const onScroll = t => {
        d = I(r);
        p = !t || !equalWH(d, _);
        if (t) {
          stopPropagation(t);
          if (p && !v) {
            i(v);
            v = l(onResized);
          }
        } else {
          onResized(t === false);
        }
        reset();
      };
      const g = [ appendChildren(t, c), addEventListener(u, e, onScroll), addEventListener(a, e, onScroll) ];
      addClass(t, et);
      setStyles(f, {
        [C]: s,
        [$]: s
      });
      l(reset);
      return [ o ? bind(onScroll, false) : reset, g ];
    }
  }
}))();

const getShowNativeOverlaidScrollbars = (t, n) => {
  const {M: o} = n;
  const [s, e] = t("showNativeOverlaidScrollbars");
  return [ s && o.x && o.y, e ];
};

const overflowIsVisible = t => t.indexOf(x) === 0;

const overflowBehaviorToOverflowStyle = t => t.replace(`${x}-`, "");

const overflowCssValueToOverflowStyle = (t, n) => {
  if (t === "auto") {
    return n ? E : H;
  }
  const o = t || H;
  return [ H, E, x ].includes(o) ? o : H;
};

const getElementOverflowStyle = (t, n) => {
  const {overflowX: o, overflowY: s} = getStyles(t, [ m, O ]);
  return {
    x: overflowCssValueToOverflowStyle(o, n.x),
    y: overflowCssValueToOverflowStyle(s, n.y)
  };
};

const xt = "__osScrollbarsHidingPlugin";

const Ht = /* @__PURE__ */ (() => ({
  [xt]: {
    static: () => ({
      R: (t, n, o, s, e) => {
        const {V: c, L: r} = t;
        const {P: i, M: l, U: a} = s;
        const u = !c && !i && (l.x || l.y);
        const [f] = getShowNativeOverlaidScrollbars(e, s);
        const _getViewportOverflowHideOffset = t => {
          const n = i || f ? 0 : 42;
          const getHideOffsetPerAxis = (t, o, s) => {
            const e = t ? n : s;
            const c = o && !i ? e : 0;
            const r = t && !!n;
            return [ c, r ];
          };
          const [o, s] = getHideOffsetPerAxis(l.x, t.x === E, a.x);
          const [e, c] = getHideOffsetPerAxis(l.y, t.y === E, a.y);
          return {
            N: {
              x: o,
              y: e
            },
            q: {
              x: s,
              y: c
            }
          };
        };
        const _hideNativeScrollbars = t => {
          if (!c) {
            const {B: s} = o;
            const e = assignDeep({}, {
              [w]: 0,
              [S]: 0,
              [y]: 0
            });
            const {N: c, q: r} = _getViewportOverflowHideOffset(t);
            const {x: i, y: l} = r;
            const {x: a, y: f} = c;
            const {F: _} = n;
            const d = s ? y : w;
            const p = s ? h : g;
            const v = _[d];
            const m = _[S];
            const O = _[p];
            const $ = _[b];
            e[C] = `calc(100% + ${f + v * -1}px)`;
            e[d] = -f + v;
            e[S] = -a + m;
            if (u) {
              e[p] = O + (l ? f : 0);
              e[b] = $ + (i ? a : 0);
            }
            return e;
          }
        };
        const _arrangeViewport = (t, s, e) => {
          if (u) {
            const {F: c} = n;
            const {N: i, q: l} = _getViewportOverflowHideOffset(t);
            const {x: a, y: u} = l;
            const {x: f, y: _} = i;
            const {B: d} = o;
            const p = d ? g : h;
            const v = c[p];
            const b = c.paddingTop;
            const y = s.w + e.w;
            const w = s.h + e.h;
            const S = {
              w: _ && u ? `${_ + y - v}px` : "",
              h: f && a ? `${f + w - b}px` : ""
            };
            setStyles(r, {
              "--os-vaw": S.w,
              "--os-vah": S.h
            });
          }
          return u;
        };
        const _undoViewportArrange = () => {
          if (u) {
            const {j: t, F: o} = n;
            const s = getElementOverflowStyle(r, t);
            const {q: e} = _getViewportOverflowHideOffset(s);
            const {x: c, y: i} = e;
            const l = {};
            const assignProps = t => each(t, (t => {
              l[t] = o[t];
            }));
            if (c) {
              assignProps([ S, v, b ]);
            }
            if (i) {
              assignProps([ y, w, h, g ]);
            }
            const a = getStyles(r, keys(l));
            const u = removeAttrClass(r, j, W);
            setStyles(r, l);
            return () => {
              setStyles(r, assignDeep({}, a, _hideNativeScrollbars(s)));
              u();
            };
          }
          return noop;
        };
        return {
          X: _arrangeViewport,
          Y: _undoViewportArrange,
          W: _hideNativeScrollbars
        };
      }
    })
  }
}))();

const Et = "__osClickScrollPlugin";

const Dt = /* @__PURE__ */ (() => ({
  [Et]: {
    static: () => (t, n, o, s, e, c, r, i) => {
      let l = false;
      let a = noop;
      const u = {
        clickScrollDistance: e,
        clickScrollDuration: 200,
        clickPressDelay: 150,
        pressDistanceDuration: 90
      };
      const easeOutQuad = t => 1 - (1 - t) * (1 - t);
      const easeInOutQuad = t => t < .5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
      const {clickScrollDistance: f, clickScrollDuration: _, clickPressDelay: d, pressDistanceDuration: p} = assignDeep({}, u, isFunction(c) ? c(r) : u);
      const v = f === 0;
      const g = p * 2.3;
      const h = p * 2.5;
      const b = f ? e / f : 0;
      const [y, w] = selfClearTimeout(Math.max(22, d));
      const S = o();
      const m = Math.sign(s);
      const O = animateNumber(0, v ? s : f * m, _, ((e, c, r) => {
        if (v) {
          n(e);
        } else {
          t(e);
        }
        if (r) {
          i(l);
          y((() => {
            if (l || v || !p) {
              return;
            }
            const t = o();
            const e = t - S;
            const c = e * b;
            const r = s - e;
            const i = c ? r / c : 0;
            const u = i <= 2.2;
            const f = Math.max(1, i || 0);
            const _ = (!i || i > .5) && Math.sign(r) === m;
            if (_) {
              a = animateNumber(e, u ? s : s - c, u ? g * f : p * f, ((t, o, e) => {
                n(t);
                if (e && !u) {
                  a = animateNumber(t, s, h, n, easeOutQuad);
                }
              }), u && easeInOutQuad);
            }
          }));
        }
      }), easeInOutQuad);
      return t => {
        l = true;
        if (t) {
          O();
        }
        w();
        a();
      };
    }
  }
}))();

const opsStringify = t => JSON.stringify(t, ((t, n) => {
  if (isFunction(n)) {
    throw 0;
  }
  return n;
}));

const getPropByPath = (t, n) => t ? `${n}`.split(".").reduce(((t, n) => t && hasOwnProperty(t, n) ? t[n] : void 0), t) : void 0;

const zt = [ 0, 33 ];

const It = [ 33, 99 ];

const At = [ 222, 666, true ];

const Tt = {
  paddingAbsolute: false,
  showNativeOverlaidScrollbars: false,
  update: {
    elementEvents: [ [ "img", "load" ] ],
    debounce: {
      mutation: zt,
      resize: null,
      event: It,
      env: At
    },
    attributes: null,
    ignoreMutation: null,
    flowDirectionStyles: null
  },
  overflow: {
    x: "scroll",
    y: "scroll"
  },
  scrollbars: {
    theme: "os-theme-dark",
    visibility: "auto",
    autoHide: "never",
    autoHideDelay: 1300,
    autoHideSuspend: false,
    dragScroll: true,
    clickScroll: false,
    pointers: [ "mouse", "touch", "pen" ]
  }
};

const getOptionsDiff = (t, n) => {
  const o = {};
  const s = concat(keys(n), keys(t));
  each(s, (s => {
    const e = t[s];
    const c = n[s];
    if (isObject(e) && isObject(c)) {
      assignDeep(o[s] = {}, getOptionsDiff(e, c));
      if (isEmptyObject(o[s])) {
        delete o[s];
      }
    } else if (hasOwnProperty(n, s) && c !== e) {
      let t = true;
      if (isArray(e) || isArray(c)) {
        try {
          if (opsStringify(e) === opsStringify(c)) {
            t = false;
          }
        } catch (r) {}
      }
      if (t) {
        o[s] = c;
      }
    }
  }));
  return o;
};

const createOptionCheck = (t, n, o) => s => [ getPropByPath(t, s), o || getPropByPath(n, s) !== void 0 ];

let kt;

const getNonce = () => kt;

const setNonce = t => {
  kt = t;
};

let Mt;

const createEnvironment = () => {
  const getNativeScrollbarSize = (t, n, o) => {
    appendChildren(document.body, t);
    appendChildren(document.body, t);
    const s = A(t);
    const e = I(t);
    const c = getFractionalSize(n);
    if (o) {
      removeElements(t);
    }
    return {
      x: e.h - s.h + c.h,
      y: e.w - s.w + c.w
    };
  };
  const getNativeScrollbarsHiding = t => {
    let n = false;
    const o = addClass(t, P);
    try {
      n = getStyles(t, "scrollbar-width") === "none" || getStyles(t, "display", "::-webkit-scrollbar") === "none";
    } catch (s) {}
    o();
    return n;
  };
  const t = `.${L}{scroll-behavior:auto!important;position:fixed;opacity:0;visibility:hidden;overflow:scroll;height:200px;width:200px;z-index:-1}.${L} div{width:200%;height:200%;margin:10px 0}.${P}{scrollbar-width:none!important}.${P}::-webkit-scrollbar,.${P}::-webkit-scrollbar-corner{appearance:none!important;display:none!important;width:0!important;height:0!important}`;
  const o = createDOM(`<div class="${L}"><div></div><style>${t}</style></div>`);
  const s = o[0];
  const e = s.firstChild;
  const c = s.lastChild;
  const r = getNonce();
  if (r) {
    c.nonce = r;
  }
  const [i, , l] = createEventListenerHub();
  const [a, u] = createCache({
    o: getNativeScrollbarSize(s, e),
    i: equalXY
  }, bind(getNativeScrollbarSize, s, e, true));
  const [f] = u();
  const _ = getNativeScrollbarsHiding(s);
  const d = {
    x: f.x === 0,
    y: f.y === 0
  };
  const v = {
    elements: {
      host: null,
      padding: !_,
      viewport: t => _ && isBodyElement(t) && t,
      content: false
    },
    scrollbars: {
      slot: true
    },
    cancel: {
      nativeScrollbarsOverlaid: false,
      body: null
    }
  };
  const g = assignDeep({}, Tt);
  const h = bind(assignDeep, {}, g);
  const b = bind(assignDeep, {}, v);
  const y = {
    U: f,
    M: d,
    P: _,
    J: !!p,
    G: bind(i, "r"),
    K: b,
    Z: t => assignDeep(v, t) && b(),
    tt: h,
    nt: t => assignDeep(g, t) && h(),
    ot: assignDeep({}, v),
    st: assignDeep({}, g)
  };
  removeAttrs(s, "style");
  removeElements(s);
  addEventListener(n, "resize", (() => {
    l("r", []);
  }));
  if (isFunction(n.matchMedia) && !_ && (!d.x || !d.y)) {
    const addZoomListener = t => {
      const o = n.matchMedia(`(resolution: ${n.devicePixelRatio}dppx)`);
      addEventListener(o, "change", (() => {
        t();
        addZoomListener(t);
      }), {
        A: true
      });
    };
    addZoomListener((() => {
      const [t, n] = a();
      assignDeep(y.U, t);
      l("r", [ n ]);
    }));
  }
  return y;
};

const getEnvironment = () => {
  if (!Mt) {
    Mt = createEnvironment();
  }
  return Mt;
};

const createEventContentChange = (t, n, o) => {
  let s = false;
  const e = o ? new WeakMap : false;
  const destroy = () => {
    s = true;
  };
  const updateElements = c => {
    if (e && o) {
      const r = o.map((n => {
        const [o, s] = n || [];
        const e = s && o ? (c || find)(o, t) : [];
        return [ e, s ];
      }));
      each(r, (o => each(o[0], (c => {
        const r = o[1];
        const i = e.get(c) || [];
        const l = t.contains(c);
        if (l && r) {
          const t = addEventListener(c, r, (o => {
            if (s) {
              t();
              e.delete(c);
            } else {
              n(o);
            }
          }));
          e.set(c, push(i, t));
        } else {
          runEachAndClear(i);
          e.delete(c);
        }
      }))));
    }
  };
  updateElements();
  return [ destroy, updateElements ];
};

const createDOMObserver = (t, n, o, s) => {
  let e = false;
  const {et: c, ct: r, rt: i, it: l, lt: a, ut: u} = s || {};
  const [_, d] = createEventContentChange(t, (() => e && o(true)), i);
  const p = c || [];
  const v = r || [];
  const g = concat(p, v);
  const observerCallback = (e, c) => {
    if (!isEmptyArray(c)) {
      const r = a || noop;
      const i = u || noop;
      const f = [];
      const _ = [];
      let p = false;
      let g = false;
      each(c, (o => {
        const {attributeName: e, target: c, type: a, oldValue: u, addedNodes: d, removedNodes: h} = o;
        const b = a === "attributes";
        const y = a === "childList";
        const w = t === c;
        const S = b && e;
        const m = S && getAttr(c, e || "");
        const O = isString(m) ? m : null;
        const C = S && u !== O;
        const $ = inArray(v, e) && C;
        if (n && (y || !w)) {
          const n = b && C;
          const a = n && l && is(c, l);
          const _ = a ? !r(c, e, u, O) : !b || n;
          const p = _ && !i(o, !!a, t, s);
          each(d, (t => push(f, t)));
          each(h, (t => push(f, t)));
          g = g || p;
        }
        if (!n && w && C && !r(c, e, u, O)) {
          push(_, e);
          p = p || $;
        }
      }));
      d((t => deduplicateArray(f).reduce(((n, o) => {
        push(n, find(t, o));
        return is(o, t) ? push(n, o) : n;
      }), [])));
      if (n) {
        if (!e && g) {
          o(false);
        }
        return [ false ];
      }
      if (!isEmptyArray(_) || p) {
        const t = [ deduplicateArray(_), p ];
        if (!e) {
          o.apply(0, t);
        }
        return t;
      }
    }
  };
  const h = new f(bind(observerCallback, false));
  return [ () => {
    h.observe(t, {
      attributes: true,
      attributeOldValue: true,
      attributeFilter: g,
      subtree: n,
      childList: n,
      characterData: n
    });
    e = true;
    return () => {
      if (e) {
        _();
        h.disconnect();
        e = false;
      }
    };
  }, () => {
    if (e) {
      return observerCallback(true, h.takeRecords());
    }
  } ];
};

let Rt = null;

const createSizeObserver = (t, n, o) => {
  const {ft: s} = o || {};
  const e = getStaticPluginModuleInstance(Ct);
  const [c] = createCache({
    o: false,
    u: true
  });
  return () => {
    const o = [];
    const r = createDOM(`<div class="${nt}"><div class="${st}"></div></div>`);
    const i = r[0];
    const l = i.firstChild;
    const onSizeChangedCallbackProxy = t => {
      const o = isArray(t) && !isEmptyArray(t);
      let s = false;
      let e = false;
      if (o) {
        const n = t[0];
        const [o, , r] = c(n.contentRect);
        const i = domRectHasDimensions(o);
        e = domRectAppeared(o, r);
        s = !e && !i;
      } else {
        e = t === true;
      }
      if (!s) {
        n({
          _t: true,
          ft: e
        });
      }
    };
    if (d) {
      if (!isBoolean(Rt)) {
        const n = new d(noop);
        n.observe(t, {
          get box() {
            Rt = true;
          }
        });
        Rt = Rt || false;
        n.disconnect();
      }
      const n = debounce(onSizeChangedCallbackProxy, {
        p: 0,
        v: 0
      });
      const resizeObserverCallback = t => n(t);
      const s = new d(resizeObserverCallback);
      s.observe(Rt ? t : l);
      push(o, [ () => {
        s.disconnect();
      }, !Rt && appendChildren(t, i) ]);
      if (Rt) {
        const n = new d(resizeObserverCallback);
        n.observe(t, {
          box: "border-box"
        });
        push(o, (() => n.disconnect()));
      }
    } else if (e) {
      const [n, c] = e(l, onSizeChangedCallbackProxy, s);
      push(o, concat([ addClass(i, ot), addEventListener(i, "animationstart", n), appendChildren(t, i) ], c));
    } else {
      return noop;
    }
    return bind(runEachAndClear, o);
  };
};

const createTrinsicObserver = (t, n) => {
  let o;
  const isHeightIntrinsic = t => t.h === 0 || t.isIntersecting || t.intersectionRatio > 0;
  const s = createDiv(it);
  const [e] = createCache({
    o: false
  });
  const triggerOnTrinsicChangedCallback = (t, o) => {
    if (t) {
      const s = e(isHeightIntrinsic(t));
      const [, c] = s;
      return c && !o && n(s) && [ s ];
    }
  };
  const intersectionObserverCallback = (t, n) => triggerOnTrinsicChangedCallback(n.pop(), t);
  return [ () => {
    const n = [];
    if (_) {
      o = new _(bind(intersectionObserverCallback, false), {
        root: t
      });
      o.observe(s);
      push(n, (() => {
        o.disconnect();
      }));
    } else {
      const onSizeChanged = () => {
        const t = I(s);
        triggerOnTrinsicChangedCallback(t);
      };
      push(n, createSizeObserver(s, onSizeChanged)());
      onSizeChanged();
    }
    return bind(runEachAndClear, push(n, appendChildren(t, s)));
  }, () => o && intersectionObserverCallback(true, o.takeRecords()) ];
};

const createObserversSetup = (t, n, o, s) => {
  let e;
  let c;
  let r;
  let i;
  let l;
  let a;
  let u;
  let f;
  const _ = `[${B}]`;
  const p = `[${j}]`;
  const v = [ "id", "class", "style", "open", "wrap", "cols", "rows" ];
  const {dt: g, vt: h, L: b, gt: y, ht: w, V: S, bt: m, yt: O, wt: C, St: $} = t;
  const getDirectionIsRTL = t => getStyles(t, "direction") === "rtl";
  const createDebouncedObservesUpdate = () => {
    let t;
    let n;
    let o;
    const e = debounce(s, {
      p: () => t,
      v: () => n,
      S: () => o,
      m(t, n) {
        const [o] = t;
        const [s] = n;
        return [ concat(keys(o), keys(s)).reduce(((t, n) => {
          t[n] = o[n] || s[n];
          return t;
        }), {}) ];
      }
    });
    const fn = (s, c) => {
      if (isArray(c)) {
        const [s, e, r] = c;
        t = s;
        n = e;
        o = r;
      } else if (isNumber(c)) {
        t = c;
        n = false;
        o = false;
      } else {
        t = false;
        n = false;
        o = false;
      }
      e(s);
    };
    fn.O = e.O;
    return fn;
  };
  const x = {
    Ot: false,
    B: getDirectionIsRTL(g)
  };
  const H = getEnvironment();
  const E = getStaticPluginModuleInstance(xt);
  const [D] = createCache({
    i: equalWH,
    o: {
      w: 0,
      h: 0
    }
  }, (() => {
    const s = E && E.R(t, n, x, H, o).Y;
    const e = m && S;
    const c = !e && hasAttrClass(h, B, N);
    const r = !S && O(W);
    const i = r && getElementScroll(y);
    const l = i && $();
    const a = C(J, c);
    const u = r && s && s();
    const f = T(b);
    const _ = getFractionalSize(b);
    if (u) {
      u();
    }
    scrollElementTo(y, i);
    if (l) {
      l();
    }
    if (c) {
      a();
    }
    return {
      w: f.w + _.w,
      h: f.h + _.h
    };
  }));
  const z = createDebouncedObservesUpdate();
  const setDirection = t => {
    const n = getDirectionIsRTL(g);
    assignDeep(t, {
      Ct: f !== n
    });
    assignDeep(x, {
      B: n
    });
    f = n;
  };
  const onTrinsicChanged = (t, n) => {
    const [o, e] = t;
    const c = {
      $t: e
    };
    assignDeep(x, {
      Ot: o
    });
    if (!n) {
      s(c);
    }
    return c;
  };
  const onSizeChanged = ({_t: t, ft: n}) => {
    const o = n ? s : z;
    const e = {
      _t: t || n,
      ft: n
    };
    setDirection(e);
    o(e, c);
  };
  const onContentMutation = (t, n) => {
    const [, o] = D();
    const s = {
      xt: o
    };
    setDirection(s);
    if (o && !n) {
      z(s, t ? r : e);
    }
    return s;
  };
  const onHostMutation = (t, n, o) => {
    const s = {
      Ht: n
    };
    setDirection(s);
    if (n && !o) {
      z(s, e);
    }
    return s;
  };
  const [I, A] = w ? createTrinsicObserver(h, onTrinsicChanged) : [];
  const k = !S && createSizeObserver(h, onSizeChanged, {
    ft: true
  });
  const [M, R] = createDOMObserver(h, false, onHostMutation, {
    ct: v,
    et: v
  });
  const V = S && d && new d((t => {
    const n = t[t.length - 1].contentRect;
    onSizeChanged({
      _t: true,
      ft: domRectAppeared(n, u)
    });
    u = n;
  }));
  return [ () => {
    if (V) {
      V.observe(h);
    }
    const t = k && k();
    const n = I && I();
    const o = M();
    const s = H.G((t => {
      const [, n] = D();
      z({
        Et: t,
        xt: n,
        _t: m
      }, i);
    }));
    return () => {
      if (V) {
        V.disconnect();
      }
      if (t) {
        t();
      }
      if (n) {
        n();
      }
      if (a) {
        a();
      }
      o();
      s();
    };
  }, ({Dt: t, zt: n, It: o}) => {
    const s = {};
    const [u] = t("update.ignoreMutation");
    const [f, d] = t("update.attributes");
    const [g, h] = t("update.elementEvents");
    const [y, m] = t("update.debounce");
    const O = h || d;
    const C = n || o;
    const ignoreMutationFromOptions = t => isFunction(u) && !!u(t);
    if (O) {
      if (l) {
        l();
      }
      if (a) {
        a();
      }
      const [t, n] = createDOMObserver(w || b, true, onContentMutation, {
        et: concat(v, f || []),
        rt: g,
        it: _,
        ut: (t, n) => {
          const {target: o, attributeName: s} = t;
          const e = !n && s && !S ? liesBetween(o, _, p) : false;
          return e || !!closest(o, `.${at}`) || ignoreMutationFromOptions(t);
        }
      });
      a = t();
      l = n;
    }
    if (m) {
      z.O();
      if (isArray(y) || isNumber(y)) {
        e = y;
        c = false;
        r = It;
        i = At;
      } else if (isPlainObject(y)) {
        e = y.mutation;
        c = y.resize;
        r = y.event;
        i = y.env;
      } else {
        e = false;
        c = false;
        r = false;
        i = false;
      }
    }
    if (C) {
      const t = R();
      const n = A && A();
      const o = l && l();
      if (t) {
        assignDeep(s, onHostMutation(t[0], t[1], C));
      }
      if (n) {
        assignDeep(s, onTrinsicChanged(n[0], C));
      }
      if (o) {
        assignDeep(s, onContentMutation(o[0], C));
      }
    }
    setDirection(s);
    return s;
  }, x ];
};

const resolveInitialization = (t, n) => isFunction(n) ? n.apply(0, t) : n;

const staticInitializationElement = (t, n, o, s) => {
  const e = isUndefined(s) ? o : s;
  const c = resolveInitialization(t, e);
  return c || n.apply(0, t);
};

const dynamicInitializationElement = (t, n, o, s) => {
  const e = isUndefined(s) ? o : s;
  const c = resolveInitialization(t, e);
  return !!c && (isHTMLElement(c) ? c : n.apply(0, t));
};

const cancelInitialization = (t, n) => {
  const {nativeScrollbarsOverlaid: o, body: s} = n || {};
  const {M: e, P: c, K: r} = getEnvironment();
  const {nativeScrollbarsOverlaid: i, body: l} = r().cancel;
  const a = o != null ? o : i;
  const u = isUndefined(s) ? l : s;
  const f = (e.x || e.y) && a;
  const _ = t && (isNull(u) ? !c : u);
  return !!f || !!_;
};

const createScrollbarsSetupElements = (t, n, o, s) => {
  const e = "--os-viewport-percent";
  const c = "--os-scroll-percent";
  const r = "--os-scroll-direction";
  const {K: i} = getEnvironment();
  const {scrollbars: l} = i();
  const {slot: a} = l;
  const {dt: u, vt: f, L: _, At: d, gt: v, bt: g, V: h} = n;
  const {scrollbars: b} = d ? {} : t;
  const {slot: y} = b || {};
  const w = [];
  const S = [];
  const m = [];
  const O = dynamicInitializationElement([ u, f, _ ], (() => h && g ? u : f), a, y);
  const initScrollTimeline = t => {
    if (p) {
      let n = null;
      let s = [];
      const e = new p({
        source: v,
        axis: t
      });
      const cancelAnimation = () => {
        if (n) {
          n.cancel();
        }
        n = null;
      };
      const _setScrollPercentAnimation = c => {
        const {Tt: r} = o;
        const i = isDefaultDirectionScrollCoordinates(r)[t];
        const l = t === "x";
        const a = [ getTrasformTranslateValue(0, l), getTrasformTranslateValue(`calc(-100% + 100cq${l ? "w" : "h"})`, l) ];
        const u = i ? a : a.reverse();
        if (s[0] === u[0] && s[1] === u[1]) {
          return cancelAnimation;
        }
        s = u;
        cancelAnimation();
        n = c.kt.animate({
          clear: [ "left" ],
          transform: u
        }, {
          timeline: e
        });
        return cancelAnimation;
      };
      return {
        Mt: _setScrollPercentAnimation
      };
    }
  };
  const C = {
    x: initScrollTimeline("x"),
    y: initScrollTimeline("y")
  };
  const getViewportPercent = () => {
    const {Rt: t, Vt: n} = o;
    const getAxisValue = (t, n) => capNumber(0, 1, t / (t + n) || 0);
    return {
      x: getAxisValue(n.x, t.x),
      y: getAxisValue(n.y, t.y)
    };
  };
  const scrollbarStructureAddRemoveClass = (t, n, o) => {
    const s = o ? addClass : removeClass;
    each(t, (t => {
      s(t.Lt, n);
    }));
  };
  const scrollbarStyle = (t, n) => {
    each(t, (t => {
      const [o, s] = n(t);
      setStyles(o, s);
    }));
  };
  const scrollbarsAddRemoveClass = (t, n, o) => {
    const s = isBoolean(o);
    const e = s ? o : true;
    const c = s ? !o : true;
    if (e) {
      scrollbarStructureAddRemoveClass(S, t, n);
    }
    if (c) {
      scrollbarStructureAddRemoveClass(m, t, n);
    }
  };
  const refreshScrollbarsHandleLength = () => {
    const t = getViewportPercent();
    const createScrollbarStyleFn = t => n => [ n.Lt, {
      [e]: roundCssNumber(t) + ""
    } ];
    scrollbarStyle(S, createScrollbarStyleFn(t.x));
    scrollbarStyle(m, createScrollbarStyleFn(t.y));
  };
  const refreshScrollbarsHandleOffset = () => {
    if (!p) {
      const {Tt: t} = o;
      const n = getScrollCoordinatesPercent(t, getElementScroll(v));
      const createScrollbarStyleFn = t => n => [ n.Lt, {
        [c]: roundCssNumber(t) + ""
      } ];
      scrollbarStyle(S, createScrollbarStyleFn(n.x));
      scrollbarStyle(m, createScrollbarStyleFn(n.y));
    }
  };
  const refreshScrollbarsScrollCoordinates = () => {
    const {Tt: t} = o;
    const n = isDefaultDirectionScrollCoordinates(t);
    const createScrollbarStyleFn = t => n => [ n.Lt, {
      [r]: t ? "0" : "1"
    } ];
    scrollbarStyle(S, createScrollbarStyleFn(n.x));
    scrollbarStyle(m, createScrollbarStyleFn(n.y));
    if (p) {
      S.forEach(C.x.Mt);
      m.forEach(C.y.Mt);
    }
  };
  const refreshScrollbarsScrollbarOffset = () => {
    if (h && !g) {
      const {Rt: t, Tt: n} = o;
      const s = isDefaultDirectionScrollCoordinates(n);
      const e = getScrollCoordinatesPercent(n, getElementScroll(v));
      const styleScrollbarPosition = n => {
        const {Lt: o} = n;
        const c = parent(o) === _ && o;
        const getTranslateValue = (t, n, o) => {
          const s = n * t;
          return numberToCssPx(o ? s : -s);
        };
        return [ c, c && {
          transform: getTrasformTranslateValue({
            x: getTranslateValue(e.x, t.x, s.x),
            y: getTranslateValue(e.y, t.y, s.y)
          })
        } ];
      };
      scrollbarStyle(S, styleScrollbarPosition);
      scrollbarStyle(m, styleScrollbarPosition);
    }
  };
  const generateScrollbarDOM = t => {
    const n = t ? "x" : "y";
    const o = t ? ft : _t;
    const e = createDiv(`${at} ${o}`);
    const c = createDiv(dt);
    const r = createDiv(pt);
    const i = {
      Lt: e,
      Pt: c,
      kt: r
    };
    const l = C[n];
    push(t ? S : m, i);
    push(w, [ appendChildren(e, c), appendChildren(c, r), bind(removeElements, e), l && l.Mt(i), s(i, scrollbarsAddRemoveClass, t) ]);
    return i;
  };
  const $ = bind(generateScrollbarDOM, true);
  const x = bind(generateScrollbarDOM, false);
  const appendElements = () => {
    appendChildren(O, S[0].Lt);
    appendChildren(O, m[0].Lt);
    return bind(runEachAndClear, w);
  };
  $();
  x();
  return [ {
    Ut: refreshScrollbarsHandleLength,
    Nt: refreshScrollbarsHandleOffset,
    qt: refreshScrollbarsScrollCoordinates,
    Bt: refreshScrollbarsScrollbarOffset,
    Ft: scrollbarsAddRemoveClass,
    jt: {
      Xt: S,
      Yt: $,
      Wt: bind(scrollbarStyle, S)
    },
    Jt: {
      Xt: m,
      Yt: x,
      Wt: bind(scrollbarStyle, m)
    }
  }, appendElements ];
};

const createScrollbarsSetupEvents = (t, n, o, s, r) => (i, l, u) => {
  const {vt: f, L: _, V: d, gt: p, Gt: v, St: g} = n;
  const {Lt: h, Pt: b, kt: y} = i;
  const [w, S] = selfClearTimeout(333);
  const [m, O] = selfClearTimeout(444);
  const scrollOffsetElementScrollBy = t => {
    if (isFunction(p.scrollBy)) {
      p.scrollBy({
        behavior: "smooth",
        left: t.x,
        top: t.y
      });
    }
  };
  const createInteractiveScrollEvents = () => {
    const n = "pointerup pointercancel lostpointercapture";
    const r = `client${u ? "X" : "Y"}`;
    const i = u ? C : $;
    const l = u ? "left" : "top";
    const a = u ? "w" : "h";
    const f = u ? "x" : "y";
    const _ = [];
    return addEventListener(b, "pointerdown", s((s => {
      const d = closest(s.target, `.${pt}`) === y;
      const h = d ? y : b;
      const w = t.scrollbars;
      const S = w[d ? "dragScroll" : "clickScroll"];
      const {button: C, isPrimary: $, pointerType: x} = s;
      const {pointers: H} = w;
      const E = C === 0 && $ && S && (H || []).includes(x);
      if (E) {
        runEachAndClear(_);
        O();
        const t = !d && (s.shiftKey || S === "instant");
        const w = bind(getBoundingClientRect, y);
        const C = bind(getBoundingClientRect, b);
        const getHandleOffset = (t, n) => (t || w())[l] - (n || C())[l];
        const $ = e(getBoundingClientRect(p)[i]) / I(p)[a] || 1;
        const x = getElementScroll(p)[f];
        const scrollRelative = t => {
          scrollElementTo(p, {
            [f]: x + t
          });
        };
        const moveHandleRelative = t => {
          const {Rt: n} = o;
          const s = I(b)[a] - I(y)[a];
          const e = 1 / $ * t / s;
          scrollRelative(e * n[f]);
        };
        const H = s[r];
        const E = w();
        const D = C();
        const z = E[i];
        const A = getHandleOffset(E, D) + z / 2;
        const T = H - D[l];
        const k = T - A;
        const M = d ? 0 : k;
        const releasePointerCapture = t => {
          runEachAndClear(L);
          h.releasePointerCapture(t.pointerId);
        };
        const R = d || t;
        const V = g();
        const L = [ addEventListener(v, n, releasePointerCapture), addEventListener(v, "selectstart", (t => preventDefault(t)), {
          D: false
        }), addEventListener(b, n, releasePointerCapture), R && addEventListener(b, "pointermove", (t => moveHandleRelative(M + t[r] - H))), R && (() => {
          const t = getElementScroll(p);
          V();
          const n = getElementScroll(p);
          const o = {
            x: n.x - t.x,
            y: n.y - t.y
          };
          if (c(o.x) > 3 || c(o.y) > 3) {
            g();
            scrollElementTo(p, t);
            scrollOffsetElementScrollBy(o);
            m(V);
          }
        }) ];
        h.setPointerCapture(s.pointerId);
        if (t) {
          moveHandleRelative(k);
        } else if (!d) {
          const t = getStaticPluginModuleInstance(Et);
          if (t) {
            const {Vt: n} = o;
            const s = t(scrollRelative, moveHandleRelative, bind(getHandleOffset), k, n[f], S, !!u, (t => {
              if (t) {
                V();
              } else {
                push(L, V);
              }
            }));
            push(L, s);
            push(_, bind(s, true));
          }
        }
      }
    })));
  };
  let x = true;
  return bind(runEachAndClear, [ addEventListener(y, "pointermove pointerleave", s(r)), addEventListener(h, "pointerenter", s((() => {
    l(ht, true);
  }))), addEventListener(h, "pointerleave pointercancel", s((() => {
    l(ht, false);
  }))), addEventListener(h, "wheel", s((t => {
    const {deltaX: n, deltaY: o, deltaMode: s} = t;
    if (x && s === 0 && parent(h) === f) {
      scrollOffsetElementScrollBy({
        x: n,
        y: o
      });
    }
    x = false;
    l(St, true);
    w((() => {
      x = true;
      l(St);
    }));
    preventDefault(t);
  })), {
    D: false,
    I: true
  }), !d && addEventListener(h, "mousedown", s((() => {
    const t = getFocusedElement();
    if (hasAttr(t, j) || hasAttr(t, B) || t === document.body) {
      a(bind(focusElement, _), 25);
    }
  }))), addEventListener(h, "pointerdown", (() => {
    const t = addEventListener(v, "click", (t => {
      n();
      stopAndPrevent(t);
    }), {
      A: true,
      I: true,
      D: false
    });
    const n = addEventListener(v, "pointerup pointercancel", (() => {
      n();
      setTimeout(t, 150);
    }), {
      I: true,
      D: true
    });
  }), {
    I: true,
    D: true
  }), createInteractiveScrollEvents(), S, O ]);
};

const createScrollbarsSetup = (t, n, o, s, e, c, r) => {
  let i;
  let l;
  let a;
  let u;
  let f;
  let _ = noop;
  let d = 0;
  const p = [ "mouse", "pen" ];
  const skipEventIfSleeping = t => n => {
    if (!o.Kt) {
      t(n);
    }
  };
  const isHoverablePointerType = t => p.includes(t.pointerType);
  const [v, g] = selfClearTimeout();
  const [h, b] = selfClearTimeout(100);
  const [y, w] = selfClearTimeout(50);
  const [S, m] = selfClearTimeout((() => d));
  const [O, C] = createScrollbarsSetupElements(t, c, e, createScrollbarsSetupEvents(n, c, e, skipEventIfSleeping, (t => isHoverablePointerType(t) && manageScrollbarsAutoHideInstantInteraction())));
  const {vt: $, Qt: H, bt: D} = c;
  const {Ft: z, Ut: I, Nt: A, qt: T, Bt: k} = O;
  const manageScrollbarsAutoHide = (t, n) => {
    m();
    const hide = t => {
      if (o.Kt) {
        return;
      }
      z(wt, t);
    };
    if (t) {
      hide();
    } else {
      const t = a ? !i : true;
      if (d > 0 && !n) {
        S(bind(hide, t));
      } else {
        hide(t);
      }
    }
  };
  const manageScrollbarsAutoHideInstantInteraction = () => {
    if (a ? !i : !u) {
      manageScrollbarsAutoHide(true);
      h((() => {
        manageScrollbarsAutoHide(false);
      }));
    }
  };
  const onHostMouseEnter = t => {
    if (isHoverablePointerType(t)) {
      i = true;
      if (!o.Kt && a) {
        manageScrollbarsAutoHide(true);
      }
    }
  };
  const onHostMouseLeave = t => {
    if (isHoverablePointerType(t)) {
      i = false;
      if (!o.Kt && a) {
        manageScrollbarsAutoHide(false);
      }
    }
  };
  const manageAutoHideSuspension = t => {
    z(yt, t, true);
    z(yt, t, false);
  };
  const M = [ m, b, w, g, () => _(), addEventListener($, "pointerover", onHostMouseEnter, {
    A: true
  }), addEventListener($, "pointerenter", onHostMouseEnter), addEventListener($, "pointerleave", onHostMouseLeave), addEventListener($, "pointermove", skipEventIfSleeping((t => {
    if (isHoverablePointerType(t) && l) {
      manageScrollbarsAutoHideInstantInteraction();
    }
  }))), addEventListener(H, "scroll", skipEventIfSleeping((t => {
    v((() => {
      A();
      manageScrollbarsAutoHideInstantInteraction();
    }));
    r(t);
    k();
  }))) ];
  const R = getStaticPluginModuleInstance(xt);
  return [ () => bind(runEachAndClear, push(M, C())), ({Dt: t, It: n, Zt: o, tn: c}) => {
    const {nn: r, sn: i, en: p, cn: v} = c || {};
    const {Ct: g, ft: h} = o || {};
    const {B: b} = s;
    const {M: w, P: S} = getEnvironment();
    const {rn: m, j: O} = e;
    const [C, $] = t("showNativeOverlaidScrollbars");
    const [M, V] = t("scrollbars.theme");
    const [L, P] = t("scrollbars.visibility");
    const [U, N] = t("scrollbars.autoHide");
    const [q, B] = t("scrollbars.autoHideSuspend");
    const [F] = t("scrollbars.autoHideDelay");
    const [j, X] = t("scrollbars.dragScroll");
    const [Y, W] = t("scrollbars.clickScroll");
    const [J, G] = t("overflow");
    const K = h && !n;
    const Q = r || i || v || g || n;
    const Z = p || P || G;
    const tt = C && w.x && w.y;
    const nt = !S && !R;
    const ot = tt || nt;
    const setScrollbarVisibility = (t, n, o) => {
      const s = t.includes(E) && (L === x || L === "auto" && n === E);
      z(vt, s, o);
      return s;
    };
    d = F;
    if ($ || nt) {
      z(lt, ot);
    }
    if (V) {
      z(f);
      z(M, true);
      f = M;
    }
    if (B || K) {
      manageAutoHideSuspension(!q);
      if (K && q) {
        if (O.x || O.y) {
          _();
          y((() => {
            _ = addEventListener(H, E, skipEventIfSleeping(bind(manageAutoHideSuspension, true)), {
              A: true
            });
          }));
        } else {
          manageAutoHideSuspension(true);
        }
      }
    }
    if (N) {
      l = U === "move";
      a = U === "leave";
      u = U === "never";
      manageScrollbarsAutoHide(u, true);
    }
    if (X) {
      z(Ot, j);
    }
    if (W) {
      z(mt, !!Y);
    }
    if (Z) {
      const t = setScrollbarVisibility(J.x, m.x, true);
      const n = setScrollbarVisibility(J.y, m.y, false);
      const o = t && n;
      z(gt, !o);
    }
    if (Q) {
      A();
      I();
      k();
      if (v) {
        T();
      }
      z(bt, !O.x, true);
      z(bt, !O.y, false);
      z(ut, b && !D);
    }
  }, {}, O ];
};

const createStructureSetupElements = t => {
  const o = getEnvironment();
  const {K: s, P: e} = o;
  const {elements: c} = s();
  const {padding: r, viewport: i, content: l} = c;
  const a = isHTMLElement(t);
  const u = a ? {} : t;
  const {elements: f} = u;
  const {padding: _, viewport: d, content: p} = f || {};
  const v = a ? t : u.target;
  const g = isBodyElement(v);
  const h = v.ownerDocument;
  const b = h.documentElement;
  const getDocumentWindow = () => h.defaultView || n;
  const y = bind(staticInitializationElement, [ v ]);
  const w = bind(dynamicInitializationElement, [ v ]);
  const S = bind(createDiv, "");
  const C = bind(y, S, i);
  const $ = bind(w, S, l);
  const elementHasOverflow = t => {
    const n = I(t);
    const o = T(t);
    const s = getStyles(t, m);
    const e = getStyles(t, O);
    return o.w - n.w > 0 && !overflowIsVisible(s) || o.h - n.h > 0 && !overflowIsVisible(e);
  };
  const x = C(d);
  const H = x === v;
  const E = H && g;
  const D = !H && $(p);
  const z = !H && x === D;
  const A = E ? b : x;
  const k = E ? A : v;
  const M = !H && w(S, r, _);
  const R = !z && D;
  const V = [ R, A, M, k ].map((t => isHTMLElement(t) && !parent(t) && t));
  const elementIsGenerated = t => t && inArray(V, t);
  const L = !elementIsGenerated(A) && elementHasOverflow(A) ? A : v;
  const P = E ? b : A;
  const N = E ? h : A;
  const X = {
    dt: v,
    vt: k,
    L: A,
    ln: M,
    ht: R,
    gt: P,
    Qt: N,
    an: g ? b : L,
    Gt: h,
    bt: g,
    At: a,
    V: H,
    un: getDocumentWindow,
    yt: t => hasAttrClass(A, j, t),
    wt: (t, n) => addRemoveAttrClass(A, j, t, n),
    St: () => addRemoveAttrClass(P, j, G, true)
  };
  const {dt: Y, vt: W, ln: J, L: Q, ht: nt} = X;
  const ot = [ () => {
    removeAttrs(W, [ B, U ]);
    removeAttrs(Y, U);
    if (g) {
      removeAttrs(b, [ U, B ]);
    }
  } ];
  let st = contents([ nt, Q, J, W, Y ].find((t => t && !elementIsGenerated(t))));
  const et = E ? Y : nt || Q;
  const ct = bind(runEachAndClear, ot);
  const appendElements = () => {
    const t = getDocumentWindow();
    const n = getFocusedElement();
    const unwrap = t => {
      appendChildren(parent(t), contents(t));
      removeElements(t);
    };
    const prepareWrapUnwrapFocus = t => addEventListener(t, "focusin focusout focus blur", stopAndPrevent, {
      I: true,
      D: false
    });
    const o = "tabindex";
    const s = getAttr(Q, o);
    const c = prepareWrapUnwrapFocus(n);
    setAttrs(W, B, H ? "" : F);
    setAttrs(J, Z, "");
    setAttrs(Q, j, "");
    setAttrs(nt, tt, "");
    if (!H) {
      setAttrs(Q, o, s || "-1");
      if (g) {
        setAttrs(b, q, "");
      }
    }
    appendChildren(et, st);
    appendChildren(W, J);
    appendChildren(J || W, !H && Q);
    appendChildren(Q, nt);
    push(ot, [ c, () => {
      const t = getFocusedElement();
      const n = elementIsGenerated(Q);
      const e = n && t === Q ? Y : t;
      const c = prepareWrapUnwrapFocus(e);
      removeAttrs(J, Z);
      removeAttrs(nt, tt);
      removeAttrs(Q, j);
      if (g) {
        removeAttrs(b, q);
      }
      if (s) {
        setAttrs(Q, o, s);
      } else {
        removeAttrs(Q, o);
      }
      if (elementIsGenerated(nt)) {
        unwrap(nt);
      }
      if (n) {
        unwrap(Q);
      }
      if (elementIsGenerated(J)) {
        unwrap(J);
      }
      focusElement(e);
      c();
    } ]);
    if (e && !H) {
      addAttrClass(Q, j, K);
      push(ot, bind(removeAttrs, Q, j));
    }
    focusElement(!H && g && n === Y && t.top === t ? Q : n);
    c();
    st = 0;
    return ct;
  };
  return [ X, appendElements, ct ];
};

const createTrinsicUpdateSegment = ({ht: t}) => ({Zt: n, fn: o, It: s}) => {
  const {$t: e} = n || {};
  const {Ot: c} = o;
  const r = t && (e || s);
  if (r) {
    setStyles(t, {
      [$]: c && "100%"
    });
  }
};

const createPaddingUpdateSegment = ({vt: t, ln: n, L: o, V: s}, e) => {
  const [c, r] = createCache({
    i: equalTRBL,
    o: topRightBottomLeft()
  }, bind(topRightBottomLeft, t, "padding", ""));
  return ({Dt: t, Zt: i, fn: l, It: a}) => {
    let [u, f] = r(a);
    const {P: _} = getEnvironment();
    const {_t: d, xt: p, Ct: m} = i || {};
    const {B: O} = l;
    const [$, x] = t("paddingAbsolute");
    const H = a || p;
    if (d || f || H) {
      [u, f] = c(a);
    }
    const E = !s && (x || m || f);
    if (E) {
      const t = !$ || !n && !_;
      const s = u.r + u.l;
      const c = u.t + u.b;
      const r = {
        [w]: t && !O ? -s : 0,
        [S]: t ? -c : 0,
        [y]: t && O ? -s : 0,
        top: t ? -u.t : 0,
        right: t ? O ? -u.r : "auto" : 0,
        left: t ? O ? "auto" : -u.l : 0,
        [C]: t && `calc(100% + ${s}px)`
      };
      const i = {
        [v]: t ? u.t : 0,
        [g]: t ? u.r : 0,
        [b]: t ? u.b : 0,
        [h]: t ? u.l : 0
      };
      setStyles(n || o, r);
      setStyles(o, i);
      assignDeep(e, {
        ln: u,
        _n: !t,
        F: n ? i : assignDeep({}, r, i)
      });
    }
    return {
      dn: E
    };
  };
};

const createOverflowUpdateSegment = (t, s) => {
  const e = getEnvironment();
  const {vt: r, ln: i, L: a, V: u, Qt: f, gt: _, bt: d, wt: p, un: v} = t;
  const {P: g} = e;
  const h = d && u;
  const b = bind(o, 0);
  const y = {
    display: () => false,
    direction: t => t !== "ltr",
    flexDirection: t => t.endsWith("-reverse"),
    writingMode: t => t !== "horizontal-tb"
  };
  const w = keys(y);
  const S = {
    i: equalWH,
    o: {
      w: 0,
      h: 0
    }
  };
  const m = {
    i: equalXY,
    o: {}
  };
  const setMeasuringMode = t => {
    p(J, !h && t);
  };
  const getFlowDirectionStyles = () => getStyles(a, w);
  const getMeasuredScrollCoordinates = (t, n) => {
    const o = !keys(t).length;
    const s = n ? true : w.some((n => {
      const o = t[n];
      return isString(o) && y[n](o);
    }));
    if (o || !s || !hasDimensions(a)) {
      return {
        T: {
          x: 0,
          y: 0
        },
        k: {
          x: 1,
          y: 1
        }
      };
    }
    setMeasuringMode(true);
    const e = getElementScroll(_);
    const r = addEventListener(f, E, (t => {
      const n = getElementScroll(_);
      if (t.isTrusted && n.x === e.x && n.y === e.y) {
        stopPropagation(t);
      }
    }), {
      I: true,
      A: true
    });
    const i = p(Q, true);
    scrollElementTo(_, {
      x: 0,
      y: 0
    });
    i();
    const u = getElementScroll(_);
    const d = T(_);
    scrollElementTo(_, {
      x: d.w,
      y: d.h
    });
    const v = getElementScroll(_);
    const g = {
      x: v.x - u.x,
      y: v.y - u.y
    };
    scrollElementTo(_, {
      x: -d.w,
      y: -d.h
    });
    const h = getElementScroll(_);
    const b = {
      x: h.x - u.x,
      y: h.y - u.y
    };
    const S = {
      x: c(g.x) >= c(b.x) ? v.x : h.x,
      y: c(g.y) >= c(b.y) ? v.y : h.y
    };
    scrollElementTo(_, e);
    l((() => r()));
    return {
      T: u,
      k: S
    };
  };
  const getOverflowAmount = (t, o) => {
    const s = n.devicePixelRatio % 1 !== 0 ? 1 : 0;
    const e = {
      w: b(t.w - o.w),
      h: b(t.h - o.h)
    };
    return {
      w: e.w > s ? e.w : 0,
      h: e.h > s ? e.h : 0
    };
  };
  const getViewportOverflowStyle = (t, n) => {
    const getAxisOverflowStyle = (t, n, o, s) => {
      const e = t === x ? H : overflowBehaviorToOverflowStyle(t);
      const c = overflowIsVisible(t);
      const r = overflowIsVisible(o);
      if (!n && !s) {
        return H;
      }
      if (c && r) {
        return x;
      }
      if (c) {
        const t = n ? x : H;
        return n && s ? e : t;
      }
      const i = r && s ? x : H;
      return n ? e : i;
    };
    return {
      x: getAxisOverflowStyle(n.x, t.x, n.y, t.y),
      y: getAxisOverflowStyle(n.y, t.y, n.x, t.x)
    };
  };
  const setViewportOverflowStyle = t => {
    const createAllOverflowStyleClassNames = t => [ x, H, E ].map((n => createViewportOverflowStyleClassName(overflowCssValueToOverflowStyle(n), t)));
    const n = createAllOverflowStyleClassNames(true).concat(createAllOverflowStyleClassNames()).join(" ");
    p(n);
    p(keys(t).map((n => createViewportOverflowStyleClassName(t[n], n === "x"))).join(" "), true);
  };
  const [O, C] = createCache(S, bind(getFractionalSize, a));
  const [$, D] = createCache(S, bind(T, a));
  const [z, I] = createCache(S);
  const [k] = createCache(m);
  const [M, R] = createCache(S);
  const [V] = createCache(m);
  const [L] = createCache({
    i: (t, n) => equal(t, n, deduplicateArray(concat(keys(t), keys(n)))),
    o: {}
  });
  const [P, U] = createCache({
    i: (t, n) => equalXY(t.T, n.T) && equalXY(t.k, n.k),
    o: getZeroScrollCoordinates()
  });
  const q = getStaticPluginModuleInstance(xt);
  const createViewportOverflowStyleClassName = (t, n) => {
    const o = n ? X : Y;
    return `${o}${capitalizeFirstLetter(t)}`;
  };
  return ({Dt: n, Zt: o, fn: c, It: l}, {dn: u}) => {
    const {_t: f, Ht: _, xt: d, Ct: y, ft: w, Et: S} = o || {};
    const m = q && q.R(t, s, c, e, n);
    const {X: x, Y: H, W: E} = m || {};
    const [T, F] = getShowNativeOverlaidScrollbars(n, e);
    const [j, X] = n("overflow");
    const Y = overflowIsVisible(j.x);
    const W = overflowIsVisible(j.y);
    const J = f || u || d || y || S || F;
    let G = C(l);
    let Q = D(l);
    let tt = I(l);
    let nt = R(l);
    if (F && g) {
      p(K, !T);
    }
    if (J) {
      if (hasAttrClass(r, B, N)) {
        setMeasuringMode(true);
      }
      const t = H && H();
      const [n] = G = O(l);
      const [o] = Q = $(l);
      const s = A(a);
      const e = h && getWindowSize(v());
      const c = {
        w: b(o.w + n.w),
        h: b(o.h + n.h)
      };
      const i = {
        w: b((e ? e.w : s.w + b(s.w - o.w)) + n.w),
        h: b((e ? e.h : s.h + b(s.h - o.h)) + n.h)
      };
      if (t) {
        t();
      }
      nt = M(i);
      tt = z(getOverflowAmount(c, i), l);
    }
    const [ot, st] = nt;
    const [et, ct] = tt;
    const [rt, it] = Q;
    const [lt, at] = G;
    const [ut] = k({
      x: et.w > 0,
      y: et.h > 0
    });
    const ft = Y && W && (ut.x || ut.y) || Y && ut.x && !ut.y || W && ut.y && !ut.x;
    const _t = u || y || S || at || it || st || ct || X || F || J || _ && h;
    const [dt] = n("update.flowDirectionStyles");
    const [pt, vt] = L(dt ? dt(a) || {} : getFlowDirectionStyles(), l);
    const gt = y || w || vt || l;
    const [ht, bt] = gt ? P(getMeasuredScrollCoordinates(pt, !!dt), l) : U();
    let yt = getViewportOverflowStyle(ut, j);
    setMeasuringMode(false);
    if (_t) {
      setViewportOverflowStyle(yt);
      yt = getElementOverflowStyle(a, ut);
      if (E && x) {
        x(yt, rt, lt);
        setStyles(a, E(yt));
      }
    }
    const [wt, St] = V(yt);
    addRemoveAttrClass(r, B, N, ft);
    addRemoveAttrClass(i, Z, N, ft);
    assignDeep(s, {
      rn: wt,
      Vt: {
        x: ot.w,
        y: ot.h
      },
      Rt: {
        x: et.w,
        y: et.h
      },
      j: ut,
      Tt: sanitizeScrollCoordinates(ht, et)
    });
    return {
      en: St,
      nn: st,
      sn: ct,
      cn: bt || ct
    };
  };
};

const createStructureSetup = t => {
  const [n, o, s] = createStructureSetupElements(t);
  const e = {
    ln: {
      t: 0,
      r: 0,
      b: 0,
      l: 0
    },
    _n: false,
    F: {
      [w]: 0,
      [S]: 0,
      [y]: 0,
      [v]: 0,
      [g]: 0,
      [b]: 0,
      [h]: 0
    },
    Vt: {
      x: 0,
      y: 0
    },
    Rt: {
      x: 0,
      y: 0
    },
    rn: {
      x: H,
      y: H
    },
    j: {
      x: false,
      y: false
    },
    Tt: getZeroScrollCoordinates()
  };
  const {dt: c, gt: r, V: i, St: l} = n;
  const {P: a, M: u} = getEnvironment();
  const f = !a && (u.x || u.y);
  const _ = [ createTrinsicUpdateSegment(n), createPaddingUpdateSegment(n, e), createOverflowUpdateSegment(n, e) ];
  return [ o, t => {
    const n = {};
    const o = f;
    const s = o && getElementScroll(r);
    const e = s && l();
    each(_, (o => {
      assignDeep(n, o(t, n) || {});
    }));
    scrollElementTo(r, s);
    if (e) {
      e();
    }
    if (!i) {
      scrollElementTo(c, 0);
    }
    return n;
  }, e, n, s ];
};

const createSetups = (t, n, o, s) => {
  let e = false;
  const c = {
    Kt: false,
    pn: false
  };
  const r = createOptionCheck(n, {});
  const [i, l, a, u, f] = createStructureSetup(t);
  const [_, d, p] = createObserversSetup(u, a, r, (t => {
    update({}, t);
  }));
  const [v, g, , h] = createScrollbarsSetup(t, n, c, p, a, u, s);
  const updateHintsAreTruthy = t => keys(t).some((n => !!t[n]));
  const update = (t, s) => {
    const {Kt: r, pn: i} = c;
    if (i || r && e) {
      return false;
    }
    const {vn: a, It: u, zt: f} = t;
    const _ = a || {};
    const v = !!u || !e;
    const h = {
      Dt: createOptionCheck(n, _, v),
      vn: _,
      It: v
    };
    const b = s || d(assignDeep({}, h, {
      zt: f
    }));
    const y = l(assignDeep({}, h, {
      fn: p,
      Zt: b
    }));
    g(assignDeep({}, h, {
      Zt: b,
      tn: y
    }));
    const w = updateHintsAreTruthy(b);
    const S = updateHintsAreTruthy(y);
    const m = w || S || !isEmptyObject(_) || v;
    e = true;
    if (m) {
      o(t, {
        Zt: b,
        tn: y
      });
    }
    return m;
  };
  return [ () => {
    const {an: t, gt: n, St: o} = u;
    const s = getElementScroll(t);
    const e = [ _(), i(), v(), () => {
      c.pn = true;
    } ];
    const r = o();
    scrollElementTo(n, s);
    r();
    return bind(runEachAndClear, e);
  }, update, t => {
    const n = c.Kt;
    c.Kt = t;
    if (!t && n !== t) {
      update({
        It: true,
        zt: true
      });
    }
  }, () => {
    g({
      Dt: createOptionCheck(n, {}, false),
      vn: {},
      It: false
    });
  }, () => ({
    gn: c,
    hn: p,
    bn: a
  }), {
    yn: u,
    wn: h
  }, f ];
};

const Vt = new WeakMap;

const addInstance = (t, n) => {
  Vt.set(t, n);
};

const removeInstance = t => {
  Vt.delete(t);
};

const getInstance$1 = t => Vt.get(t);

const OverlayScrollbars = (t, n, o) => {
  const {tt: s} = getEnvironment();
  const e = isHTMLElement(t);
  const c = e ? t : t.target;
  const r = getInstance$1(c);
  if (n && !r) {
    const r = [];
    const i = {};
    const validateOptions = t => {
      const n = removeUndefinedProperties(t);
      const o = getStaticPluginModuleInstance(R);
      return o ? o(n, true) : n;
    };
    const l = assignDeep({}, s(), validateOptions(n));
    const [a, u, f] = createEventListenerHub();
    const [_, d, p] = createEventListenerHub(o);
    const triggerEvent = (t, n) => {
      p(t, n);
      f(t, n);
    };
    const [v, g, h, b, y, w, S] = createSetups(t, l, (({vn: t, It: n}, {Zt: o, tn: s}) => {
      const {_t: e, Ct: c, $t: r, xt: i, Ht: l, ft: a} = o;
      const {nn: u, sn: f, en: _, cn: d} = s;
      triggerEvent("updated", [ m, {
        updateHints: {
          sizeChanged: !!e,
          directionChanged: !!c,
          heightIntrinsicChanged: !!r,
          overflowEdgeChanged: !!u,
          overflowAmountChanged: !!f,
          overflowStyleChanged: !!_,
          scrollCoordinatesChanged: !!d,
          contentMutation: !!i,
          hostMutation: !!l,
          appear: !!a
        },
        changedOptions: t || {},
        force: !!n
      } ]);
    }), (t => triggerEvent("scroll", [ m, t ])));
    const destroy = t => {
      const {gn: n} = y();
      const {pn: o} = n;
      if (o) {
        return;
      }
      removeInstance(c);
      runEachAndClear(r);
      triggerEvent("destroyed", [ m, t ]);
      u();
      d();
    };
    const update = t => g({
      It: t,
      zt: true
    });
    const m = {
      options(t, n) {
        if (t) {
          const o = n ? s() : {};
          const e = getOptionsDiff(l, assignDeep(o, validateOptions(t)));
          if (!isEmptyObject(e)) {
            assignDeep(l, e);
            g({
              vn: e
            });
          }
        }
        return assignDeep({}, l);
      },
      on: _,
      off: (t, n) => {
        if (t && n) {
          d(t, n);
        }
      },
      state() {
        const {gn: t, hn: n, bn: o} = y();
        const {pn: s, Kt: e} = t;
        const {B: c} = n;
        const {Vt: r, Rt: i, rn: l, j: a, ln: u, _n: f, Tt: _} = o;
        return assignDeep({}, {
          overflowEdge: r,
          overflowAmount: i,
          overflowStyle: l,
          hasOverflow: a,
          scrollCoordinates: {
            start: _.T,
            end: _.k
          },
          padding: u,
          paddingAbsolute: f,
          directionRTL: c,
          sleeping: e,
          destroyed: s
        });
      },
      elements() {
        const {dt: t, vt: n, ln: o, L: s, ht: e, gt: c, Qt: r} = w.yn;
        const {jt: i, Jt: l} = w.wn;
        const translateScrollbarStructure = t => {
          const {kt: n, Pt: o, Lt: s} = t;
          return {
            scrollbar: s,
            track: o,
            handle: n
          };
        };
        const translateScrollbarsSetupElement = t => {
          const {Xt: n, Yt: o} = t;
          const s = translateScrollbarStructure(n[0]);
          return assignDeep({}, s, {
            clone: () => {
              const t = translateScrollbarStructure(o());
              b();
              return t;
            }
          });
        };
        return assignDeep({}, {
          target: t,
          host: n,
          padding: o || s,
          viewport: s,
          content: e || s,
          scrollOffsetElement: c,
          scrollEventElement: r,
          scrollbarHorizontal: translateScrollbarsSetupElement(i),
          scrollbarVertical: translateScrollbarsSetupElement(l)
        });
      },
      update: update,
      destroy: bind(destroy, false),
      sleep: h,
      plugin: t => i[keys(t)[0]]
    };
    push(r, [ S ]);
    addInstance(c, m);
    registerPluginModuleInstances(k, OverlayScrollbars, [ m, a, i ]);
    if (cancelInitialization(w.yn.bt, !e && t.cancel)) {
      destroy(true);
      return m;
    }
    push(r, v());
    triggerEvent("initialized", [ m ]);
    m.update();
    return m;
  }
  return r;
};

OverlayScrollbars.plugin = t => {
  const n = isArray(t);
  const o = n ? t : [ t ];
  const s = o.map((t => registerPluginModuleInstances(t, OverlayScrollbars)[0]));
  addPlugins(o);
  return n ? s : s[0];
};

OverlayScrollbars.valid = t => {
  const n = t && t.elements;
  const o = isFunction(n) && n();
  return isPlainObject(o) && !!getInstance$1(o.target);
};

OverlayScrollbars.env = () => {
  const {U: t, M: n, P: o, J: s, ot: e, st: c, K: r, Z: i, tt: l, nt: a} = getEnvironment();
  return assignDeep({}, {
    scrollbarsSize: t,
    scrollbarsOverlaid: n,
    scrollbarsHiding: o,
    scrollTimeline: s,
    staticDefaultInitialization: e,
    staticDefaultOptions: c,
    getDefaultInitialization: r,
    setDefaultInitialization: i,
    getDefaultOptions: l,
    setDefaultOptions: a
  });
};

OverlayScrollbars.nonce = setNonce;

OverlayScrollbars.trustedTypePolicy = setTrustedTypePolicy;

/**
 * Scrollbar Manager - OverlayScrollbars Integration
 *
 * Provides centralized scrollbar management for Lithium using OverlayScrollbars.
 * This module handles initialization, configuration, and cleanup for scrollable
 * components including Tabulator tables, CodeMirror, SunEditor, and popups.
 *
 * Features:
 * - Consistent cross-browser scrollbar styling (Chrome, Firefox, Safari, Edge)
 * - Rounded track and thumb with inset effect
 * - Automatic cleanup on component destruction
 * - Theme-aware styling via CSS variables
 *
 * Usage:
 *   import { scrollbarManager } from '../../core/scrollbar-manager.js';
 *
 *   // Initialize for Tabulator (called automatically by LithiumTable)
 *   const osInstance = scrollbarManager.initTabulator(tableHolderElement);
 *
 *   // Initialize for CodeMirror
 *   const osInstance = scrollbarManager.initCodeMirror(scrollerElement);
 *
 *   // Initialize for SunEditor
 *   const osInstance = scrollbarManager.initSunEditor(editorElement);
 *
 *   // Initialize for popup/dropdown
 *   const osInstance = scrollbarManager.initPopup(popupElement);
 *
 *   // Cleanup when done
 *   scrollbarManager.destroy(osInstance);
 *
 * @module core/scrollbar-manager
 */


// ── Plugin Registration ───────────────────────────────────────────────────

// Register plugins for advanced scrollbar behavior
OverlayScrollbars.plugin(Ht);
OverlayScrollbars.plugin($t);
OverlayScrollbars.plugin(Dt);

// ── Configuration Presets ─────────────────────────────────────────────────

/**
 * Base configuration for all Lithium scrollbars
 * @type {Object}
 */
const BASE_CONFIG = {
  paddingAbsolute: true, // Reserve space for scrollbars (classic mode)
  scrollbars: {
    theme: 'os-theme-lithium',
    visibility: 'auto', // Hide when content fits, show when overflow exists
    autoHide: 'never',  // Once visible, stay visible (don't fade)
    autoHideDelay: 800,
    autoHideSuspend: false,
    dragScroll: true,
    clickScroll: true,
    pointers: ['mouse', 'touch', 'pen'],
  },
};

function applyInstanceThemeClasses(instance, themeClasses = '') {
  if (!instance || typeof themeClasses !== 'string' || themeClasses.trim() === '') {
    return;
  }

  const elements = instance.elements?.();
  const scrollbars = [
    elements?.scrollbarHorizontal?.scrollbar,
    elements?.scrollbarVertical?.scrollbar,
  ].filter(Boolean);

  if (scrollbars.length === 0) {
    return;
  }

  themeClasses
    .split(/\s+/)
    .filter(Boolean)
    .forEach((className) => {
      scrollbars.forEach((scrollbar) => scrollbar.classList.add(className));
    });
}

function attachTabulatorPerpendicularAxisPin(instance) {
  if (!instance) {
    return () => {};
  }

  const elements = instance.elements?.();
  const viewport = elements?.viewport;
  const horizontalHandle = elements?.scrollbarHorizontal?.handle;
  const verticalHandle = elements?.scrollbarVertical?.handle;

  if (!viewport || !horizontalHandle || !verticalHandle) {
    return () => {};
  }

  let dragAxis = null;
  let lockedScrollLeft = 0;
  let lockedScrollTop = 0;

  const clearDragAxis = () => {
    dragAxis = null;
  };

  const pinHorizontalDrag = () => {
    dragAxis = 'x';
    lockedScrollTop = viewport.scrollTop;
  };

  const pinVerticalDrag = () => {
    dragAxis = 'y';
    lockedScrollLeft = viewport.scrollLeft;
  };

  const enforcePinnedAxis = () => {
    if (dragAxis === 'x' && viewport.scrollTop !== lockedScrollTop) {
      viewport.scrollTop = lockedScrollTop;
    }

    if (dragAxis === 'y' && viewport.scrollLeft !== lockedScrollLeft) {
      viewport.scrollLeft = lockedScrollLeft;
    }
  };

  horizontalHandle.addEventListener('pointerdown', pinHorizontalDrag);
  verticalHandle.addEventListener('pointerdown', pinVerticalDrag);
  viewport.addEventListener('scroll', enforcePinnedAxis, { passive: true });
  window.addEventListener('pointerup', clearDragAxis);
  window.addEventListener('pointercancel', clearDragAxis);
  window.addEventListener('blur', clearDragAxis);

  return () => {
    horizontalHandle.removeEventListener('pointerdown', pinHorizontalDrag);
    verticalHandle.removeEventListener('pointerdown', pinVerticalDrag);
    viewport.removeEventListener('scroll', enforcePinnedAxis);
    window.removeEventListener('pointerup', clearDragAxis);
    window.removeEventListener('pointercancel', clearDragAxis);
    window.removeEventListener('blur', clearDragAxis);
    clearDragAxis();
  };
}

function syncTabulatorScrollbarSlot(tableHolder, slot) {
  if (!tableHolder || !slot) {
    return;
  }

  slot.style.top = `${tableHolder.offsetTop}px`;
  slot.style.left = `${tableHolder.offsetLeft}px`;
  slot.style.width = `${tableHolder.clientWidth}px`;
  slot.style.height = `${tableHolder.clientHeight}px`;
}

function ensureTabulatorScrollbarSlot(tableHolder) {
  const tabulator = tableHolder?.closest?.('.tabulator');
  if (!tabulator) {
    return { slot: null, sync: () => {}, destroy: () => {} };
  }

  let slot = tabulator.querySelector(':scope > .lithium-table-osb-slot');
  if (!slot) {
    slot = document.createElement('div');
    slot.className = 'lithium-table-osb-slot';
    slot.setAttribute('aria-hidden', 'true');
    tabulator.appendChild(slot);
  }

  const sync = () => syncTabulatorScrollbarSlot(tableHolder, slot);
  sync();

  return {
    slot,
    sync,
    destroy: () => {
      slot?.remove();
    },
  };
}

/**
 * Tabulator-specific scrollbar configuration.
 *
 * The active probe must preserve `.tabulator-tableholder` as Tabulator's
 * scrolling viewport. Reparenting or replacing the holder breaks Tabulator's
 * layout and virtual-scroll assumptions.
 *
 * To keep the DOM shape stable, `initTabulator()` initializes OSB against the
 * existing holder and reuses the existing `.tabulator-table` content element.
 * The generated scrollbar chrome stays attached to the holder so it anchors to
 * the true scroll viewport instead of the outer Tabulator shell / footer.
 *
 * Behavior:
 *   - visibility: 'auto' — scrollbar appears only when content overflows
 *   - autoHide: 'never' — never fades out while overflow exists
 *   - paddingAbsolute: true — classic (gutter) mode: scrollbar reserves its
 *     own layout space at the edge of the viewport, never overlaying content
 *   - showNativeOverlaidScrollbars: false — hide any native bar
 *
 * @type {Object}
 */
const TABULATOR_CONFIG = {
  ...BASE_CONFIG,
  paddingAbsolute: true,
  showNativeOverlaidScrollbars: false,
  overflow: { x: 'scroll', y: 'scroll' },
  scrollbars: {
    ...BASE_CONFIG.scrollbars,
    theme: 'os-theme-lithium os-theme-lithium-tabulator',
    autoHide: 'never',  // Never fade - show/hide completely instead
    autoHideDelay: 0,
    autoHideSuspend: false,
    clickScroll: true,
    pointers: ['mouse', 'touch'],
  },
};



/**
 * SunEditor-specific scrollbar configuration
 * @type {Object}
 */
const SUNEDITOR_CONFIG = {
  ...BASE_CONFIG,
  scrollbars: {
    ...BASE_CONFIG.scrollbars,
    theme: 'os-theme-lithium os-theme-lithium-suneditor',
    dragScroll: true,
    clickScroll: true,
  },
};

/**
 * Minimal/unobtrusive scrollbar configuration
 * @type {Object}
 */
const MINIMAL_CONFIG = {
  ...BASE_CONFIG,
  scrollbars: {
    ...BASE_CONFIG.scrollbars,
    theme: 'os-theme-lithium os-theme-lithium-minimal',
    autoHide: 'move',
    autoHideDelay: 500,
  },
};

/**
 * Popup/dropdown scrollbar configuration
 * Prevents scroll propagation to parent containers
 * @type {Object}
 */
const POPUP_CONFIG = {
  ...BASE_CONFIG,
  // Set paddingAbsolute: false for popups - the host element may not have
  // proper padding set, which breaks scrollbar reservation
  paddingAbsolute: false,
  // Force overflow behavior - OSB should always treat content as vertically scrollable
  // This prevents --os-viewport-overflow-y from being set to 'hidden'
  overflow: { x: 'hidden', y: 'scroll' },
  scrollbars: {
    ...BASE_CONFIG.scrollbars,
    theme: 'os-theme-lithium os-theme-lithium-popup',
    dragScroll: true,
    clickScroll: true,
    // Ensure scrollbars are visible when content overflows
    visibility: 'visible',
    autoHide: 'never',
  },
};

// ── Instance Tracking ─────────────────────────────────────────────────────

/**
 * WeakMap to track OverlayScrollbars instances for cleanup
 * @type {WeakMap<HTMLElement, OverlayScrollbars>}
 */
const instanceMap = new WeakMap();

/**
 * Track all active instances for global cleanup if needed
 * @type {Set<OverlayScrollbars>}
 */
const activeInstances = new Set();

// ── Scrollbar Manager Class ────────────────────────────────────────────────

class ScrollbarManager {
  constructor() {
    this.initialized = true;
  }

  /**
   * Initialize OverlayScrollbars on an element with the specified configuration
   *
   * @param {HTMLElement} element - The scrollable element
   * @param {Object} config - OverlayScrollbars configuration
   * @param {Object} [callbacks] - Optional callbacks for scroll events
   * @returns {OverlayScrollbars|null} The OverlayScrollbars instance or null if initialization failed
   */
  init(element, config = BASE_CONFIG, callbacks = {}) {
    if (!element) {
      console.warn('[ScrollbarManager] Cannot initialize: element is null');
      return null;
    }

    // Check if element already has an instance
    if (instanceMap.has(element)) {
      const existing = instanceMap.get(element);
      if (existing && !existing.destroyed) {
        return existing;
      }
      // Instance was destroyed, remove from map
      instanceMap.delete(element);
    }

    try {
      const themeClasses = config?.scrollbars?.theme || '';

      // Create the OverlayScrollbars instance using v2 signature:
      // OverlayScrollbars({ target, ... }, config, callbacks)
      const instance = OverlayScrollbars({
        target: element,
      }, config, {
        ...callbacks,
        initialized: (instance) => {
          applyInstanceThemeClasses(instance, themeClasses);
          instanceMap.set(element, instance);
          activeInstances.add(instance);
          if (callbacks.initialized) {
            callbacks.initialized(instance);
          }
        },
        destroyed: (instance, destroyedBy) => {
          instanceMap.delete(element);
          activeInstances.delete(instance);
          if (callbacks.destroyed) {
            callbacks.destroyed(instance, destroyedBy);
          }
        },
      });

      return instance;
    } catch (error) {
      console.error('[ScrollbarManager] Failed to initialize OverlayScrollbars:', error);
      return null;
    }
  }

  /**
   * Initialize scrollbar for a Tabulator table holder.
   *
   * Preserves `.tabulator-tableholder` as the real scroll viewport and avoids
   * reparenting it. This keeps Tabulator's row manager attached to the same
   * element it expects while letting OSB render scrollbar chrome inside the
   * holder viewport.
   *
   * @param {HTMLElement} tableHolder - The .tabulator-tableholder element
   * @param {Object} [callbacks] - Optional callbacks for scroll events
   * @returns {OverlayScrollbars|null}
   */
  initTabulator(tableHolder, callbacks = {}) {
    log(Subsystems.UI, Status.DEBUG, `initTabulator: entry, holder exists: ${!!tableHolder}, has .tabulator-table: ${!!tableHolder?.querySelector?.('.tabulator-table')}`);
    if (!tableHolder) return null;

     if (instanceMap.has(tableHolder)) {
       const existing = instanceMap.get(tableHolder);
       if (existing && !existing.destroyed) {
         log(Subsystems.UI, Status.DEBUG, `initTabulator: returning existing instance from map`);
         return existing;
       }
       log(Subsystems.UI, Status.DEBUG, `initTabulator: clearing stale/cached instance from map`);
       instanceMap.delete(tableHolder);
       if (tableHolder.__overlayscrollbars) {
         delete tableHolder.__overlayscrollbars;
       }
     }

     const scrollbarSlot = ensureTabulatorScrollbarSlot(tableHolder);
    try {
      const themeClasses = TABULATOR_CONFIG.scrollbars.theme;
      const instance = OverlayScrollbars({
        target: tableHolder,
        elements: {
          viewport: tableHolder,
          content: () => tableHolder.querySelector('.tabulator-table'),
        },
        scrollbars: {
          slot: () => ensureTabulatorScrollbarSlot(tableHolder).slot,
        },
      }, TABULATOR_CONFIG, {
        ...callbacks,
        initialized: (inst) => {
          log(Subsystems.UI, Status.DEBUG, `initTabulator: OSB initialized callback`);
          scrollbarSlot.sync();
          applyInstanceThemeClasses(inst, themeClasses);
          inst.__lithiumTabulatorAxisPinCleanup = attachTabulatorPerpendicularAxisPin(inst);
          inst.__lithiumTabulatorScrollbarSlotSync = scrollbarSlot.sync;
          inst.__lithiumTabulatorScrollbarSlotDestroy = scrollbarSlot.destroy;
          instanceMap.set(tableHolder, inst);
          activeInstances.add(inst);
          log(Subsystems.UI, Status.DEBUG, `initTabulator: instance stored in map, activeInstances size: ${activeInstances.size}`);
          if (callbacks.initialized) {
            callbacks.initialized(inst);
          }
        },
        updated: (inst, updateInfo) => {
          inst.__lithiumTabulatorScrollbarSlotSync?.();
          if (callbacks.updated) {
            callbacks.updated(inst, updateInfo);
          }
        },
        destroyed: (inst, destroyedBy) => {
          log(Subsystems.UI, Status.DEBUG, `initTabulator: OSB destroyed callback, reason: ${destroyedBy}`);
          inst.__lithiumTabulatorAxisPinCleanup?.();
          delete inst.__lithiumTabulatorAxisPinCleanup;
          inst.__lithiumTabulatorScrollbarSlotDestroy?.();
          delete inst.__lithiumTabulatorScrollbarSlotDestroy;
          delete inst.__lithiumTabulatorScrollbarSlotSync;
          instanceMap.delete(tableHolder);
          activeInstances.delete(inst);
          log(Subsystems.UI, Status.DEBUG, `initTabulator: after destroy, instanceMap size: ${instanceMap.size}, activeInstances size: ${activeInstances.size}`);
          if (callbacks.destroyed) {
            callbacks.destroyed(inst, destroyedBy);
          }
        },
      });

      log(Subsystems.UI, Status.DEBUG, `initTabulator: OverlayScrollbars() call returned instance: ${!!instance}`);
      return instance;
    } catch (error) {
      console.error('[ScrollbarManager] Failed to initialize Tabulator OverlayScrollbars:', error);
      return null;
    }
  }
  


  /**
   * Initialize scrollbar for a SunEditor instance
   *
   * @param {HTMLElement} editorElement - The SunEditor wrapper element
   * @param {Object} [callbacks] - Optional callbacks for scroll events
   * @returns {OverlayScrollbars|null}
   */
  initSunEditor(editorElement, callbacks = {}) {
    if (!editorElement) return null;

    // SunEditor wraps content in a specific structure
    // We need to target the actual scrollable container
    const scrollable = editorElement.querySelector('.se-wrapper-inner') ||
                      editorElement.querySelector('.se-wrapper') ||
                      editorElement;

    if (scrollable) {
      scrollable.classList.add('lithium-suneditor-scrollable');
    }

    return this.init(scrollable, SUNEDITOR_CONFIG, callbacks);
  }

   /**
    * Initialize scrollbar for a popup or dropdown
    * Prevents scroll propagation to parent containers
    *
    * @param {HTMLElement} element - The popup/dropdown scrollable element
    * @param {Object} [callbacks] - Optional callbacks for scroll events
    * @returns {OverlayScrollbars|null}
    *
    * Usage:
    *   const dropdown = document.querySelector('.tabulator-edit-list');
    *   const osInstance = scrollbarManager.initPopup(dropdown);
    *
    *   // Cleanup when popup closes
    *   scrollbarManager.destroy(osInstance);
    */
   initPopup(element, callbacks = {}) {
     if (!element) {
       console.error('[ScrollbarManager] initPopup: element is null/undefined');
       log(Subsystems.UI, Status.ERROR, '[initPopup] element is null/undefined');
       return null;
     }

     log(Subsystems.UI, Status.DEBUG, '[initPopup] Called for element: ' + element.className + ', tag: ' + element.tagName);

     // Add popup class for styling
     element.classList.add('lithium-popup-scrollable');

     // Initialize with popup config - use v2 signature
     // Let OSB create its own viewport/content structure
     try {
       log(Subsystems.UI, Status.DEBUG, '[initPopup] Creating OverlayScrollbars instance');
       const instance = OverlayScrollbars(
         {
           target: element,
         },
         POPUP_CONFIG,
         {
           ...callbacks,
           initialized: function(instance) {
             log(Subsystems.UI, Status.INFO, '[initPopup] OSB initialized successfully');
             // Log DOM structure
             const host = element.querySelector('.os-host');
             const viewport = element.querySelector('[data-overlayscrollbars-viewport]');
             const content = element.querySelector('[data-overlayscrollbars-content]');
             log(Subsystems.UI, Status.DEBUG, '[initPopup] DOM check - host: ' + !!host + ', viewport: ' + !!viewport + ', content: ' + !!content);
             if (viewport) {
               log(Subsystems.UI, Status.DEBUG, '[initPopup] viewport overflow-y: ' + getComputedStyle(viewport).overflowY + ', overflow-x: ' + getComputedStyle(viewport).overflowX);
             }
             // Add attribute for CSS targeting
             element.setAttribute('data-overlayscrollbars', 'popup');
             if (callbacks.initialized) {
               callbacks.initialized(instance);
             }
           },
           updated: function(instance, changes) {
             log(Subsystems.UI, Status.DEBUG, '[initPopup] OSB updated');
             if (callbacks.updated) {
               callbacks.updated(instance, changes);
             }
           },
           destroyed: function(instance, destroyedBy) {
             log(Subsystems.UI, Status.DEBUG, '[initPopup] OSB destroyed, reason: ' + destroyedBy);
             if (callbacks.destroyed) {
               callbacks.destroyed(instance, destroyedBy);
             }
           }
         }
       );

       if (instance) {
         instanceMap.set(element, instance);
         activeInstances.add(instance);
         log(Subsystems.UI, Status.INFO, '[initPopup] Instance stored, total active: ' + activeInstances.size);
       } else {
         console.error('[ScrollbarManager] initPopup: OverlayScrollbars returned null/undefined!');
         log(Subsystems.UI, Status.ERROR, '[initPopup] OverlayScrollbars returned null/undefined!');
       }

       return instance;
     } catch (error) {
       console.error('[ScrollbarManager] initPopup: Exception during initialization', error);
       log(Subsystems.UI, Status.ERROR, '[initPopup] Exception: ' + error.message);
       return null;
     }
   }


  /**
   * Initialize scrollbar for a generic element
   *
   * @param {HTMLElement} element - The scrollable element
   * @param {Object} [options] - Options for initialization
   * @param {boolean} [options.minimal=false] - Use minimal scrollbar style
   * @param {Object} [callbacks] - Optional callbacks for scroll events
   * @returns {OverlayScrollbars|null}
   */
  initGeneric(element, options = {}, callbacks = {}) {
    const config = options.minimal ? MINIMAL_CONFIG : BASE_CONFIG;
    return this.init(element, config, callbacks);
  }

  /**
   * Update an existing OverlayScrollbars instance
   * Call this after content changes that might affect scrollability
   *
   * @param {OverlayScrollbars|null} instance
   */
  update(instance) {
    if (instance && !instance.destroyed) {
      instance.__lithiumTabulatorScrollbarSlotSync?.();
      instance.update();
    }
  }

  /**
   * Destroy an OverlayScrollbars instance and clean up
   *
   * @param {OverlayScrollbars|null} instance
   */
  destroy(instance) {
    if (instance && !instance.destroyed) {
      instance.destroy();
    }
  }

  /**
   * Destroy all active OverlayScrollbars instances
   * Useful for cleanup when switching managers or logging out
   */
  destroyAll() {
    activeInstances.forEach((instance) => {
      if (!instance.destroyed) {
        instance.destroy();
      }
    });
    activeInstances.clear();
  }

  /**
   * Get the OverlayScrollbars instance for an element
   *
   * @param {HTMLElement} element
   * @returns {OverlayScrollbars|undefined}
   */
  getInstance(element) {
    return instanceMap.get(element);
  }

  /**
   * Check if an element has an OverlayScrollbars instance
   *
   * @param {HTMLElement} element
   * @returns {boolean}
   */
  hasInstance(element) {
    const instance = instanceMap.get(element);
    return instance && !instance.destroyed;
  }

  /**
   * Scroll an element to a specific position
   *
   * @param {OverlayScrollbars|null} instance
   * @param {Object} position
   * @param {number} [position.x]
   * @param {number} [position.y]
   * @param {Object} [options]
   * @param {boolean} [options.smooth=true]
   */
  scrollTo(instance, position, options = { smooth: true }) {
    if (!instance || instance.destroyed) return;

    const behavior = options.smooth ? 'smooth' : 'instant';
    const { viewport } = instance.elements();

    if (position.y !== undefined) {
      viewport.scrollTo({
        top: position.y,
        behavior,
      });
    }

    if (position.x !== undefined) {
      viewport.scrollTo({
        left: position.x,
        behavior,
      });
    }
  }

  /**
   * Get scroll position of an instance
   *
   * @param {OverlayScrollbars|null} instance
   * @returns {{x: number, y: number}|null}
   */
  getScrollPosition(instance) {
    if (!instance || instance.destroyed) return null;

    const { viewport } = instance.elements();
    return {
      x: viewport.scrollLeft,
      y: viewport.scrollTop,
    };
  }

  /**
   * Check if element is scrollable (has overflow)
   *
   * @param {HTMLElement} element
   * @returns {boolean}
   */
  isScrollable(element) {
    if (!element) return false;
    return element.scrollHeight > element.clientHeight ||
           element.scrollWidth > element.clientWidth;
  }
}

// ── Singleton Export ──────────────────────────────────────────────────────

const scrollbarManager = new ScrollbarManager();

// ── Convenience Exports ───────────────────────────────────────────────────

const {
  initTabulator,
  initSunEditor,
  initPopup,
  initGeneric,
  update,
  destroy,
  destroyAll,
  getInstance,
  hasInstance,
  scrollTo,
  getScrollPosition,
  isScrollable,
} = scrollbarManager;

export { scrollbarManager as s };
