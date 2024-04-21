var ze=Object.defineProperty;var Be=(e,t,n)=>t in e?ze(e,t,{enumerable:!0,configurable:!0,writable:!0,value:n}):e[t]=n;var te=(e,t,n)=>(Be(e,typeof t!="symbol"?t+"":t,n),n);(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const l of document.querySelectorAll('link[rel="modulepreload"]'))r(l);new MutationObserver(l=>{for(const i of l)if(i.type==="childList")for(const c of i.addedNodes)c.tagName==="LINK"&&c.rel==="modulepreload"&&r(c)}).observe(document,{childList:!0,subtree:!0});function n(l){const i={};return l.integrity&&(i.integrity=l.integrity),l.referrerPolicy&&(i.referrerPolicy=l.referrerPolicy),l.crossOrigin==="use-credentials"?i.credentials="include":l.crossOrigin==="anonymous"?i.credentials="omit":i.credentials="same-origin",i}function r(l){if(l.ep)return;l.ep=!0;const i=n(l);fetch(l.href,i)}})();function k(){}const Se=e=>e;function Fe(e){return!!e&&(typeof e=="object"||typeof e=="function")&&typeof e.then=="function"}function Pe(e){return e()}function be(){return Object.create(null)}function B(e){e.forEach(Pe)}function Z(e){return typeof e=="function"}function x(e,t){return e!=e?t==t:e!==t||e&&typeof e=="object"||typeof e=="function"}let K;function ye(e,t){return e===t?!0:(K||(K=document.createElement("a")),K.href=t,e===K.href)}function Me(e){return Object.keys(e).length===0}function Je(e,...t){if(e==null){for(const r of t)r(void 0);return k}const n=e.subscribe(...t);return n.unsubscribe?()=>n.unsubscribe():n}function Ke(e,t,n){e.$$.on_destroy.push(Je(t,n))}const Le=typeof window<"u";let Ue=Le?()=>window.performance.now():()=>Date.now(),ue=Le?e=>requestAnimationFrame(e):k;const I=new Set;function Ae(e){I.forEach(t=>{t.c(e)||(I.delete(t),t.f())}),I.size!==0&&ue(Ae)}function Ye(e){let t;return I.size===0&&ue(Ae),{promise:new Promise(n=>{I.add(t={c:e,f:n})}),abort(){I.delete(t)}}}function y(e,t){e.appendChild(t)}function Re(e){if(!e)return document;const t=e.getRootNode?e.getRootNode():e.ownerDocument;return t&&t.host?t:e.ownerDocument}function He(e){const t=b("style");return t.textContent="/* empty */",Ve(Re(e),t),t.sheet}function Ve(e,t){return y(e.head||e,t),t.sheet}function v(e,t,n){e.insertBefore(t,n||null)}function $(e){e.parentNode&&e.parentNode.removeChild(e)}function De(e,t){for(let n=0;n<e.length;n+=1)e[n]&&e[n].d(t)}function b(e){return document.createElement(e)}function P(e){return document.createTextNode(e)}function L(){return P(" ")}function fe(){return P("")}function ae(e,t,n,r){return e.addEventListener(t,n,r),()=>e.removeEventListener(t,n,r)}function Ge(e){return function(t){return t.preventDefault(),e.call(this,t)}}function j(e,t,n){n==null?e.removeAttribute(t):e.getAttribute(t)!==n&&e.setAttribute(t,n)}function Qe(e){return Array.from(e.childNodes)}function ee(e,t){t=""+t,e.data!==t&&(e.data=t)}function We(e,t,{bubbles:n=!1,cancelable:r=!1}={}){return new CustomEvent(e,{detail:t,bubbles:n,cancelable:r})}const Y=new Map;let H=0;function Xe(e){let t=5381,n=e.length;for(;n--;)t=(t<<5)-t^e.charCodeAt(n);return t>>>0}function Ze(e,t){const n={stylesheet:He(t),rules:{}};return Y.set(e,n),n}function $e(e,t,n,r,l,i,c,s=0){const o=16.666/r;let u=`{
`;for(let m=0;m<=1;m+=o){const _=t+(n-t)*i(m);u+=m*100+`%{${c(_,1-_)}}
`}const g=u+`100% {${c(n,1-n)}}
}`,d=`__svelte_${Xe(g)}_${s}`,a=Re(e),{stylesheet:w,rules:f}=Y.get(a)||Ze(a,e);f[d]||(f[d]=!0,w.insertRule(`@keyframes ${d} ${g}`,w.cssRules.length));const h=e.style.animation||"";return e.style.animation=`${h?`${h}, `:""}${d} ${r}ms linear ${l}ms 1 both`,H+=1,d}function xe(e,t){const n=(e.style.animation||"").split(", "),r=n.filter(t?i=>i.indexOf(t)<0:i=>i.indexOf("__svelte")===-1),l=n.length-r.length;l&&(e.style.animation=r.join(", "),H-=l,H||et())}function et(){ue(()=>{H||(Y.forEach(e=>{const{ownerNode:t}=e.stylesheet;t&&$(t)}),Y.clear())})}let M;function N(e){M=e}function tt(){if(!M)throw new Error("Function called outside component initialization");return M}const T=[],le=[];let q=[];const ce=[],nt=Promise.resolve();let se=!1;function rt(){se||(se=!0,nt.then(de))}function z(e){q.push(e)}function it(e){ce.push(e)}const ne=new Set;let R=0;function de(){if(R!==0)return;const e=M;do{try{for(;R<T.length;){const t=T[R];R++,N(t),lt(t.$$)}}catch(t){throw T.length=0,R=0,t}for(N(null),T.length=0,R=0;le.length;)le.pop()();for(let t=0;t<q.length;t+=1){const n=q[t];ne.has(n)||(ne.add(n),n())}q.length=0}while(T.length);for(;ce.length;)ce.pop()();se=!1,ne.clear(),N(e)}function lt(e){if(e.fragment!==null){e.update(),B(e.before_update);const t=e.dirty;e.dirty=[-1],e.fragment&&e.fragment.p(e.ctx,t),e.after_update.forEach(z)}}function ct(e){const t=[],n=[];q.forEach(r=>e.indexOf(r)===-1?t.push(r):n.push(r)),n.forEach(r=>r()),q=t}let F;function st(){return F||(F=Promise.resolve(),F.then(()=>{F=null})),F}function re(e,t,n){e.dispatchEvent(We(`${t?"intro":"outro"}${n}`))}const U=new Set;let S;function V(){S={r:0,c:[],p:S}}function G(){S.r||B(S.c),S=S.p}function O(e,t){e&&e.i&&(U.delete(e),e.i(t))}function C(e,t,n,r){if(e&&e.o){if(U.has(e))return;U.add(e),S.c.push(()=>{U.delete(e),r&&(n&&e.d(1),r())}),e.o(t)}else r&&r()}const ot={duration:0};function Q(e,t,n,r){let i=t(e,n,{direction:"both"}),c=r?0:1,s=null,o=null,u=null,g;function d(){u&&xe(e,u)}function a(f,h){const m=f.b-c;return h*=Math.abs(m),{a:c,b:f.b,d:m,duration:h,start:f.start,end:f.start+h,group:f.group}}function w(f){const{delay:h=0,duration:m=300,easing:_=Se,tick:p=k,css:E}=i||ot,A={start:Ue()+h,b:f};f||(A.group=S,S.r+=1),"inert"in e&&(f?g!==void 0&&(e.inert=g):(g=e.inert,e.inert=!0)),s||o?o=A:(E&&(d(),u=$e(e,c,f,m,h,_,E)),f&&p(0,1),s=a(A,m),z(()=>re(e,f,"start")),Ye(J=>{if(o&&J>o.start&&(s=a(o,m),o=null,re(e,s.b,"start"),E&&(d(),u=$e(e,c,s.b,s.duration,0,_,i.css))),s){if(J>=s.end)p(c=s.b,1-c),re(e,s.b,"end"),o||(s.b?d():--s.group.r||B(s.group.c)),s=null;else if(J>=s.start){const qe=J-s.start;c=s.a+s.d*_(qe/s.duration),p(c,1-c)}}return!!(s||o)}))}return{run(f){Z(i)?st().then(()=>{i=i({direction:f?"in":"out"}),w(f)}):w(f)},end(){d(),s=o=null}}}function ut(e,t){const n=t.token={};function r(l,i,c,s){if(t.token!==n)return;t.resolved=s;let o=t.ctx;c!==void 0&&(o=o.slice(),o[c]=s);const u=l&&(t.current=l)(o);let g=!1;t.block&&(t.blocks?t.blocks.forEach((d,a)=>{a!==i&&d&&(V(),C(d,1,1,()=>{t.blocks[a]===d&&(t.blocks[a]=null)}),G())}):t.block.d(1),u.c(),O(u,1),u.m(t.mount(),t.anchor),g=!0),t.block=u,t.blocks&&(t.blocks[i]=u),g&&de()}if(Fe(e)){const l=tt();if(e.then(i=>{N(l),r(t.then,1,t.value,i),N(null)},i=>{if(N(l),r(t.catch,2,t.error,i),N(null),!t.hasCatch)throw i}),t.current!==t.pending)return r(t.pending,0),!0}else{if(t.current!==t.then)return r(t.then,1,t.value,e),!0;t.resolved=e}}function ft(e,t,n){const r=t.slice(),{resolved:l}=e;e.current===e.then&&(r[e.value]=l),e.current===e.catch&&(r[e.error]=l),e.block.p(r,n)}function W(e){return(e==null?void 0:e.length)!==void 0?e:Array.from(e)}function at(e,t,n){const r=e.$$.props[t];r!==void 0&&(e.$$.bound[r]=n,n(e.$$.ctx[r]))}function Te(e){e&&e.c()}function he(e,t,n){const{fragment:r,after_update:l}=e.$$;r&&r.m(t,n),z(()=>{const i=e.$$.on_mount.map(Pe).filter(Z);e.$$.on_destroy?e.$$.on_destroy.push(...i):B(i),e.$$.on_mount=[]}),l.forEach(z)}function _e(e,t){const n=e.$$;n.fragment!==null&&(ct(n.after_update),B(n.on_destroy),n.fragment&&n.fragment.d(t),n.on_destroy=n.fragment=null,n.ctx=[])}function dt(e,t){e.$$.dirty[0]===-1&&(T.push(e),rt(),e.$$.dirty.fill(0)),e.$$.dirty[t/31|0]|=1<<t%31}function me(e,t,n,r,l,i,c=null,s=[-1]){const o=M;N(e);const u=e.$$={fragment:null,ctx:[],props:i,update:k,not_equal:l,bound:be(),on_mount:[],on_destroy:[],on_disconnect:[],before_update:[],after_update:[],context:new Map(t.context||(o?o.$$.context:[])),callbacks:be(),dirty:s,skip_bound:!1,root:t.target||o.$$.root};c&&c(u.root);let g=!1;if(u.ctx=n?n(e,t.props||{},(d,a,...w)=>{const f=w.length?w[0]:a;return u.ctx&&l(u.ctx[d],u.ctx[d]=f)&&(!u.skip_bound&&u.bound[d]&&u.bound[d](f),g&&dt(e,d)),a}):[],u.update(),g=!0,B(u.before_update),u.fragment=r?r(u.ctx):!1,t.target){if(t.hydrate){const d=Qe(t.target);u.fragment&&u.fragment.l(d),d.forEach($)}else u.fragment&&u.fragment.c();t.intro&&O(e.$$.fragment),he(e,t.target,t.anchor),de()}N(o)}class pe{constructor(){te(this,"$$");te(this,"$$set")}$destroy(){_e(this,1),this.$destroy=k}$on(t,n){if(!Z(n))return k;const r=this.$$.callbacks[t]||(this.$$.callbacks[t]=[]);return r.push(n),()=>{const l=r.indexOf(n);l!==-1&&r.splice(l,1)}}$set(t){this.$$set&&!Me(t)&&(this.$$.skip_bound=!0,this.$$set(t),this.$$.skip_bound=!1)}}const ht="4";typeof window<"u"&&(window.__svelte||(window.__svelte={v:new Set})).v.add(ht);function X(e,{delay:t=0,duration:n=400,easing:r=Se}={}){const l=+getComputedStyle(e).opacity;return{delay:t,duration:n,easing:r,css:i=>`opacity: ${i*l}`}}const ge="http://127.0.0.1:7070";function _t(e){let t,n,r,l,i,c,s,o=e[0].name+"",u,g,d,a,w;return{c(){t=b("div"),n=b("label"),r=b("img"),c=L(),s=b("span"),u=P(o),g=L(),d=b("input"),ye(r.src,l=`${e[0].class}.png`)||j(r,"src",l),j(r,"alt",i=e[0].class),j(r,"class","svelte-1wp6y8o"),j(n,"for",e[2]),j(n,"class","svelte-1wp6y8o"),j(d,"type","checkbox"),j(d,"id",e[2]),j(d,"name",e[2]),j(t,"class","svelte-1wp6y8o")},m(f,h){v(f,t,h),y(t,n),y(n,r),y(n,c),y(n,s),y(s,u),y(t,g),y(t,d),a||(w=ae(d,"change",function(){Z(e[1])&&e[1].apply(this,arguments)}),a=!0)},p(f,[h]){e=f,h&1&&!ye(r.src,l=`${e[0].class}.png`)&&j(r,"src",l),h&1&&i!==(i=e[0].class)&&j(r,"alt",i),h&1&&o!==(o=e[0].name+"")&&ee(u,o)},i:k,o:k,d(f){f&&$(t),a=!1,w()}}}function mt(e,t,n){let{character:r}=t;const l=`${r.name}`;let{onchange:i}=t;return e.$$set=c=>{"character"in c&&n(0,r=c.character),"onchange"in c&&n(1,i=c.onchange)},[r,i,l]}class pt extends pe{constructor(t){super(),me(this,t,mt,_t,x,{character:0,onchange:1})}}const D=[];function gt(e,t=k){let n;const r=new Set;function l(s){if(x(e,s)&&(e=s,n)){const o=!D.length;for(const u of r)u[1](),D.push(u,e);if(o){for(let u=0;u<D.length;u+=2)D[u][0](D[u+1]);D.length=0}}}function i(s){l(s(e))}function c(s,o=k){const u=[s,o];return r.add(u),r.size===1&&(n=t(l,i)||k),s(e),()=>{r.delete(u),r.size===0&&n&&(n(),n=null)}}return{set:l,update:i,subscribe:c}}const oe=gt(void 0);let ie;function Ie(e){oe.set({error:e}),ie!==void 0&&clearTimeout(ie),ie=setTimeout(()=>oe.set(void 0),5e3)}function we(e,t,n){const r=e.slice();return r[5]=t[n],r}function ve(e,t,n){const r=e.slice();return r[5]=t[n],r}function ke(e){let t,n;return t=new pt({props:{character:e[5],onchange:e[3]}}),{c(){Te(t.$$.fragment)},m(r,l){he(t,r,l),n=!0},p(r,l){const i={};l&1&&(i.character=r[5]),t.$set(i)},i(r){n||(O(t.$$.fragment,r),n=!0)},o(r){C(t.$$.fragment,r),n=!1},d(r){_e(t,r)}}}function Ee(e){let t,n,r;function l(s,o){return s[1].clients.length>0?yt:bt}let i=l(e),c=i(e);return{c(){t=b("div"),c.c(),j(t,"class","injection-result svelte-t4g4h9")},m(s,o){v(s,t,o),c.m(t,null),r=!0},p(s,o){i===(i=l(s))&&c?c.p(s,o):(c.d(1),c=i(s),c&&(c.c(),c.m(t,null)))},i(s){r||(s&&z(()=>{r&&(n||(n=Q(t,X,{},!0)),n.run(1))}),r=!0)},o(s){s&&(n||(n=Q(t,X,{},!1)),n.run(0)),r=!1},d(s){s&&$(t),c.d(),s&&n&&n.end()}}}function bt(e){let t;return{c(){t=b("p"),t.textContent="No clients were injected!"},m(n,r){v(n,t,r)},p:k,d(n){n&&$(t)}}}function yt(e){let t,n,r,l=W(e[1].clients),i=[];for(let c=0;c<l.length;c+=1)i[c]=je(we(e,l,c));return{c(){t=b("h3"),t.textContent="Successfully injected dll to:",n=L();for(let c=0;c<i.length;c+=1)i[c].c();r=fe()},m(c,s){v(c,t,s),v(c,n,s);for(let o=0;o<i.length;o+=1)i[o]&&i[o].m(c,s);v(c,r,s)},p(c,s){if(s&2){l=W(c[1].clients);let o;for(o=0;o<l.length;o+=1){const u=we(c,l,o);i[o]?i[o].p(u,s):(i[o]=je(u),i[o].c(),i[o].m(r.parentNode,r))}for(;o<i.length;o+=1)i[o].d(1);i.length=l.length}},d(c){c&&($(t),$(n),$(r)),De(i,c)}}}function je(e){let t,n,r=e[5].pid+"",l;return{c(){t=b("p"),n=P("Pid: "),l=P(r)},m(i,c){v(i,t,c),y(t,n),y(t,l)},p(i,c){c&2&&r!==(r=i[5].pid+"")&&ee(l,r)},d(i){i&&$(t)}}}function $t(e){let t,n,r,l,i,c,s,o,u,g,d=W(e[0].characters),a=[];for(let h=0;h<d.length;h+=1)a[h]=ke(ve(e,d,h));const w=h=>C(a[h],1,1,()=>{a[h]=null});let f=e[1].show_result&&Ee(e);return{c(){t=b("div"),n=b("form"),r=b("div");for(let h=0;h<a.length;h+=1)a[h].c();l=L(),i=b("input"),c=L(),f&&f.c(),s=fe(),j(r,"class","characters svelte-t4g4h9"),j(i,"type","submit"),i.value="Inject",j(t,"class","form-container svelte-t4g4h9")},m(h,m){v(h,t,m),y(t,n),y(n,r);for(let _=0;_<a.length;_+=1)a[_]&&a[_].m(r,null);y(n,l),y(n,i),v(h,c,m),f&&f.m(h,m),v(h,s,m),o=!0,u||(g=ae(n,"submit",Ge(e[2])),u=!0)},p(h,[m]){if(m&9){d=W(h[0].characters);let _;for(_=0;_<d.length;_+=1){const p=ve(h,d,_);a[_]?(a[_].p(p,m),O(a[_],1)):(a[_]=ke(p),a[_].c(),O(a[_],1),a[_].m(r,null))}for(V(),_=d.length;_<a.length;_+=1)w(_);G()}h[1].show_result?f?(f.p(h,m),m&2&&O(f,1)):(f=Ee(h),f.c(),O(f,1),f.m(s.parentNode,s)):f&&(V(),C(f,1,1,()=>{f=null}),G())},i(h){if(!o){for(let m=0;m<d.length;m+=1)O(a[m]);O(f),o=!0}},o(h){a=a.filter(Boolean);for(let m=0;m<a.length;m+=1)C(a[m]);C(f),o=!1},d(h){h&&($(t),$(c),$(s)),De(a,h),f&&f.d(h),u=!1,g()}}}function wt(e,t,n){let{config:r}=t,{num_selected:l=0}=t,i={show_result:!1,clients:[],timeout:void 0};async function c(o){let u=[];for(const[a,w]of new FormData(o.target).entries())w==="on"&&u.push(a);let g=await fetch(`${ge}/inject`,{method:"POST",body:JSON.stringify({enabled_characters:u})}),d=await g.json();g.ok?(i.timeout&&clearTimeout(i.timeout),n(1,i={clients:d.clients,timeout:setTimeout(()=>n(1,i={clients:[],timeout:void 0,show_result:!1}),5e3),show_result:!0})):Ie({error:`injection failed: ${d.details}`})}function s(o){o.target.checked?n(4,l+=1):n(4,l-=1)}return e.$$set=o=>{"config"in o&&n(0,r=o.config),"num_selected"in o&&n(4,l=o.num_selected)},[r,i,c,s,l]}class vt extends pe{constructor(t){super(),me(this,t,wt,$t,x,{config:0,num_selected:4})}}function Oe(e){e[5]=e[6].result}function kt(e){let t;return{c(){t=b("div"),t.textContent=`${e[7]}`},m(n,r){v(n,t,r)},p:k,i:k,o:k,d(n){n&&$(t)}}}function Et(e){Oe(e);let t,n,r,l,i,c,s,o,u,g,d,a,w,f;function h(p){e[4](p)}let m={config:e[5]};e[0]!==void 0&&(m.num_selected=e[0]),t=new vt({props:m}),le.push(()=>at(t,"num_selected",h));let _=e[1]!==void 0&&Ce(e);return{c(){Te(t.$$.fragment),r=L(),l=b("div"),i=b("button"),c=P("Launch "),s=P(e[0]),o=P(" client(s)"),g=L(),_&&_.c(),d=fe(),i.disabled=u=e[0]<1},m(p,E){he(t,p,E),v(p,r,E),v(p,l,E),y(l,i),y(i,c),y(i,s),y(i,o),v(p,g,E),_&&_.m(p,E),v(p,d,E),a=!0,w||(f=ae(i,"click",e[2]),w=!0)},p(p,E){Oe(p);const A={};!n&&E&1&&(n=!0,A.num_selected=p[0],it(()=>n=!1)),t.$set(A),(!a||E&1)&&ee(s,p[0]),(!a||E&1&&u!==(u=p[0]<1))&&(i.disabled=u),p[1]!==void 0?_?(_.p(p,E),E&2&&O(_,1)):(_=Ce(p),_.c(),O(_,1),_.m(d.parentNode,d)):_&&(V(),C(_,1,1,()=>{_=null}),G())},i(p){a||(O(t.$$.fragment,p),O(_),a=!0)},o(p){C(t.$$.fragment,p),C(_),a=!1},d(p){p&&($(r),$(l),$(g),$(d)),_e(t,p),_&&_.d(p),w=!1,f()}}}function Ce(e){let t,n,r,l=e[1].error&&Ne(e);return{c(){t=b("div"),l&&l.c(),j(t,"class","error-display")},m(i,c){v(i,t,c),l&&l.m(t,null),r=!0},p(i,c){i[1].error?l?l.p(i,c):(l=Ne(i),l.c(),l.m(t,null)):l&&(l.d(1),l=null)},i(i){r||(i&&z(()=>{r&&(n||(n=Q(t,X,{},!0)),n.run(1))}),r=!0)},o(i){i&&(n||(n=Q(t,X,{},!1)),n.run(0)),r=!1},d(i){i&&$(t),l&&l.d(),i&&n&&n.end()}}}function Ne(e){let t,n=e[1].error+"",r;return{c(){t=b("p"),r=P(n)},m(l,i){v(l,t,i),y(t,r)},p(l,i){i&2&&n!==(n=l[1].error+"")&&ee(r,n)},d(l){l&&$(t)}}}function jt(e){let t;return{c(){t=b("div"),t.textContent="Loading config..."},m(n,r){v(n,t,r)},p:k,i:k,o:k,d(n){n&&$(t)}}}function Ot(e){let t,n,r,l,i={ctx:e,current:null,token:null,hasCatch:!0,pending:jt,then:Et,catch:kt,value:6,error:7,blocks:[,,,]};return ut(fetch(`${ge}/config`).then(e[3]),i),{c(){t=b("main"),n=b("h1"),n.textContent="wowibotti_suite: Injector :D",r=L(),i.block.c()},m(c,s){v(c,t,s),y(t,n),y(t,r),i.block.m(t,i.anchor=null),i.mount=()=>t,i.anchor=null,l=!0},p(c,[s]){e=c,ft(i,e,s)},i(c){l||(O(i.block),l=!0)},o(c){for(let s=0;s<3;s+=1){const o=i.blocks[s];C(o)}l=!1},d(c){c&&$(t),i.block.d(),i.token=null,i=null}}}function Ct(e,t,n){let r;Ke(e,oe,o=>n(1,r=o));let l=0;async function i(){if(l>0){let o=await fetch(`${ge}/launch`,{method:"POST",body:JSON.stringify({num_clients:l})});const u=await o.json();o.ok||Ie(u.details)}}const c=async o=>{const u=await o.json();if(o.ok)return Promise.resolve(u);throw new Error(`Fetching config failed: HTTP ${o.status}: ${u.details}`)};function s(o){l=o,n(0,l)}return[l,r,i,c,s]}class Nt extends pe{constructor(t){super(),me(this,t,Ct,Ot,x,{})}}new Nt({target:document.getElementById("app")});
