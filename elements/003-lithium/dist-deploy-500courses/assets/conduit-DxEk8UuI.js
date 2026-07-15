import{n as e}from"./rolldown-runtime-QTnfLwEv.js";var t=null,n=null,r=null,i=null,a=null,o=null,s=null,c=null,l=null,u=0,d=null,f=null,p=!1,m=`header`,h=new Map,g=0,_=`radar-disconnected`,v=`Status: Disconnected`,y=3333,b=360/y,ee=y/2,x=300,S=400,C=5,w=32,T=32,E=27.5*.6,D=`#ffffff`,O=`#10b981`,k=`#ef4444`,A=`#ffffff`;function j(){l||(l=document.createElement(`style`),l.textContent=`
    #targets-group polygon,
    #targets-group rect,
    #targets-group-sidebar polygon,
    #targets-group-sidebar rect {
      transition: fill 0.25s ease, opacity 0.25s ease;
    }
    #targets-group g.ping polygon,
    #targets-group g.ping rect,
    #targets-group-sidebar g.ping polygon,
    #targets-group-sidebar g.ping rect {
      filter: brightness(2.0) drop-shadow(0 0 5px #ffffff);
    }
    #targets-group g.fading,
    #targets-group-sidebar g.fading {
      opacity: 0;
      transition: opacity var(--radar-fade-duration, 1.67s) linear;
    }
    #radar-icon,
    #radar-icon-sidebar {
      color: var(--accent-primary, #5C6BC0);
      transition: color 0.4s ease;
    }
    /* Sweep animation - moved to CSS for smoother performance */
    #sweep-group,
    #sweep-arm,
    #sweep-group-sidebar,
    #sweep-arm-sidebar {
      animation: sweep-rotation ${y}ms linear infinite;
      transform-origin: ${w}px ${T}px;
    }
    /* Pause sweep when disconnected */
    .radar-disconnected #sweep-group,
    .radar-disconnected #sweep-arm,
    .radar-disconnected #sweep-group-sidebar,
    .radar-disconnected #sweep-arm-sidebar {
      animation-play-state: paused;
    }
    @keyframes sweep-rotation {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
  `,document.head.appendChild(l))}function M(){return t=document.getElementById(`radar-icon`),n=document.getElementById(`radar-icon-sidebar`),r=document.getElementById(`sweep-group`),i=document.getElementById(`sweep-arm`),a=document.getElementById(`targets-group`),o=document.getElementById(`sweep-group-sidebar`),s=document.getElementById(`sweep-arm-sidebar`),c=document.getElementById(`targets-group-sidebar`),j(),N(),m=`header`,I(!1,!1),!0}function N(){i&&i.setAttribute(`stroke`,D),r&&r.querySelectorAll(`path`).forEach(e=>{e.setAttribute(`fill`,D)}),s&&s.setAttribute(`stroke`,D),o&&o.querySelectorAll(`path`).forEach(e=>{e.setAttribute(`fill`,D)})}function te(e){m=e===`sidebar`?`sidebar`:`header`;let r=m===`header`?t:n;r&&(r.classList.remove(`radar-connected`,`radar-flaky`,`radar-disconnected`),r.classList.add(_)),ne()}function ne(){let e=m===`header`?t:n;e&&(e.dataset.tooltip=v)}function P(e){v=e;let r=m===`header`?t:n;r&&(r.dataset.tooltip=e)}function F(){return v}function I(e,r=!1){p=e,_=e?r?`radar-flaky`:`radar-connected`:`radar-disconnected`,m===`header`&&t?(t.classList.remove(`radar-connected`,`radar-flaky`,`radar-disconnected`),e?r?(t.classList.add(`radar-flaky`),K()):(t.classList.add(`radar-connected`),K()):(t.classList.add(`radar-disconnected`),q(),t.animate([{transform:`rotate(0deg)`},{transform:`rotate(8deg)`},{transform:`rotate(-8deg)`},{transform:`rotate(0deg)`}],{duration:420}))):m===`sidebar`&&n&&(n.classList.remove(`radar-connected`,`radar-flaky`,`radar-disconnected`),e?r?(n.classList.add(`radar-flaky`),K()):(n.classList.add(`radar-connected`),K()):(n.classList.add(`radar-disconnected`),q(),n.animate([{transform:`rotate(0deg)`},{transform:`rotate(8deg)`},{transform:`rotate(-8deg)`},{transform:`rotate(0deg)`}],{duration:420})))}function L(){I(!0,!1)}function R(){I(!0,!0)}function z(){I(!1,!1)}function B(){p&&(m===`header`&&i?(i.setAttribute(`stroke`,`#a5f3fc`),setTimeout(()=>{i&&i.setAttribute(`stroke`,D)},140)):m===`sidebar`&&s&&(s.setAttribute(`stroke`,`#a5f3fc`),setTimeout(()=>{s&&s.setAttribute(`stroke`,D)},140)))}function V(e){let t=e*Math.PI/180;return{x:w+E*Math.sin(t),y:T-E*Math.cos(t)}}function H(e=`triangle`,t=`rest`){let n=m===`sidebar`?c:a;if(!n)return null;let r=`target-${++g}`,i=V(u),o=document.createElementNS(`http://www.w3.org/2000/svg`,`g`);o.dataset.targetId=r,o.setAttribute(`transform`,`translate(${i.x},${i.y})`);let s;e===`triangle`?(s=document.createElementNS(`http://www.w3.org/2000/svg`,`polygon`),s.setAttribute(`points`,`0,-4.5  3.5,3.5  -3.5,3.5`)):(s=document.createElementNS(`http://www.w3.org/2000/svg`,`rect`),s.setAttribute(`x`,`-3`),s.setAttribute(`y`,`-3`),s.setAttribute(`width`,`6`),s.setAttribute(`height`,`6`)),s.setAttribute(`fill`,A),o.appendChild(s),n.appendChild(o);let l=t===`ws`?O:k,d=setTimeout(()=>{s.parentNode&&s.setAttribute(`fill`,l)},x),f=u;return h.set(r,{element:o,shape:s,category:t,targetAngle:f,settleTimeout:d}),K(),r}function U(e){let t;if(e)t=h.get(e);else{let n=h.keys().next().value;n&&(e=n,t=h.get(n))}if(!t)return;let{element:n,shape:r,settleTimeout:i}=t;clearTimeout(i);let a=t.category===`ws`?O:k;r.setAttribute(`fill`,A),setTimeout(()=>{r.parentNode&&r.setAttribute(`fill`,a)},80),setTimeout(()=>{r.parentNode&&r.setAttribute(`fill`,A)},160),setTimeout(()=>{r.parentNode&&r.setAttribute(`fill`,a)},240),n.style.setProperty(`--radar-fade-duration`,`${(ee/1e3).toFixed(2)}s`),setTimeout(()=>{n.classList.add(`fading`)},320),setTimeout(()=>{n.remove(),h.delete(e),h.size===0&&!p&&q()},1986.5)}function W(e){return typeof window<`u`&&typeof window.requestAnimationFrame==`function`?window.requestAnimationFrame(e):setTimeout(e,16)}function G(e){typeof window<`u`&&typeof window.cancelAnimationFrame==`function`?window.cancelAnimationFrame(e):clearTimeout(e)}function K(){if(d)return;f=null;function e(t){if(!p&&h.size===0){d=null,f=null;return}f===null&&(f=t);let n=t-f;f=t,u=(u+b*n)%360,h.forEach(e=>{let{element:t,targetAngle:n}=e;if(Math.abs((u-n+180)%360-180)<C&&!t.classList.contains(`ping`)){t.classList.add(`ping`);let n=t.querySelector(`polygon, rect`);n&&n.setAttribute(`fill`,A),setTimeout(()=>{if(t.classList.remove(`ping`),n&&n.parentNode){let t=e.category===`ws`?O:k;n.setAttribute(`fill`,t)}},S)}}),d=W(e)}d=W(e)}function q(){d&&(G(d),d=null,f=null)}function J(){q(),h.forEach(({settleTimeout:e})=>clearTimeout(e)),h.clear(),l&&=(l.remove(),null),t=null,n=null,r=null,i=null,a=null,o=null,s=null,c=null}var Y=e({authQuery:()=>$,buildQueryPayload:()=>X,extractError:()=>Q,extractRows:()=>Z});function X(e,t={}){if(e==null||typeof e!=`number`)throw Error(`buildQueryPayload: queryRef must be a number, got ${typeof e}`);return{query_ref:e,params:t}}function Z(e){if(!e)return[];if(Array.isArray(e.data))return e.data;if(Array.isArray(e.rows))return e.rows;if(Array.isArray(e.results)&&e.results.length>0){let t=e.results[0];if(t.success&&Array.isArray(t.rows))return t.rows}return Array.isArray(e)?e:[]}function Q(e){return e&&e.success===!1?{message:e.message||e.error||`Unknown error`,error:e.error||`Unknown error`,queryRef:e.query_ref,database:e.database}:null}async function $(e,t,n={}){let r=H(`triangle`,`rest`),i=X(t,n),a;try{a=await e.post(`conduit/auth_query`,i)}catch(e){let t=Q(e.data);throw t&&(e.serverError=t),e}finally{r&&U(r)}let o=Q(a);if(o){let e=Error(o.message);throw e.serverError=o,e}return Z(a)}export{F as a,U as c,L as d,z as f,J as i,te as l,Y as n,M as o,R as p,H as r,B as s,$ as t,P as u};