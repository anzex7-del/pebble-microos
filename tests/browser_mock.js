/* Unit harness only. This does not render or automate a browser. */
const elements=new Map(), frames=[], saved=new Map();
let perf=1000;
function element(id='') { return {id,style:{setProperty(){}},classList:{add(){},remove(){},toggle(){}},textContent:'',dataset:{},value:'',open:false,children:[],append(n){this.children.push(n);},replaceChildren(){this.children=[];},setAttribute(){},addEventListener(){},click(){},focus(){},showModal(){this.open=true;},getContext(){return new Proxy({}, {get(t,k){return t[k]||(()=>{});},set(t,k,v){t[k]=v;return true;}});},toBlob(cb){cb({});}}; }
const document={documentElement:element(),getElementById(id){if(!elements.has(id))elements.set(id,element(id));return elements.get(id);},querySelectorAll(){return [];},createElement:element,addEventListener(){}};
const localStorage={getItem:k=>saved.get(k)||null,setItem:(k,v)=>saved.set(k,v)};
const performance={now:()=>perf};
function matchMedia(){return {matches:true};}
function requestAnimationFrame(fn){frames.push(fn);}
function addEventListener(){}
function setTimeout(){return 1;}
function clearTimeout(){}
const URL={createObjectURL(){return 'blob:test';},revokeObjectURL(){}};
function Blob(){}
