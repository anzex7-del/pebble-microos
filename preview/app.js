/* Independent browser experience prototype; native engine lives in firmware/src. */
'use strict';
const $=id=>document.getElementById(id), canvas=$('display'), ctx=canvas.getContext('2d');
const STORE='pebble.microos.v1';
const palette=['#bfe6a0','#a4d6f2','#fac6b1','#d0bbf3'];
const defaults=()=>({version:1,name:'Milo',food:80,joy:80,energy:90,xp:0,focuses:0,games:0,theme:0,sleeping:false,finish:'moss',savedAt:Date.now(),timer:{remaining:0,duration:0,running:false,complete:false,rewarded:false}});
const clamp=n=>Math.max(0,Math.min(100,n));
let state=defaults(), storageAvailable=true;
try { const p=JSON.parse(localStorage.getItem(STORE)); if(p&&p.version===1){
  for(const k of ['food','joy','energy']) if(Number.isFinite(p[k])) state[k]=clamp(p[k]);
  for(const k of ['xp','focuses','games']) if(Number.isInteger(p[k])&&p[k]>=0) state[k]=Math.min(p[k],10000000);
  if(typeof p.name==='string'&&p.name.trim()) state.name=p.name.trim().slice(0,16);
  if(Number.isInteger(p.theme)&&p.theme>=0&&p.theme<4) state.theme=p.theme;
  state.sleeping=p.sleeping===true;
  if(['moss','milk','ink'].includes(p.finish)) state.finish=p.finish;
  if(p.timer&&Number.isFinite(p.timer.remaining)&&Number.isFinite(p.timer.duration)&&p.timer.duration>0&&p.timer.duration<=86400000&&p.timer.remaining>=0&&p.timer.remaining<=p.timer.duration) {
    state.timer={remaining:p.timer.remaining,duration:p.timer.duration,running:p.timer.running===true,complete:p.timer.complete===true,rewarded:p.timer.rewarded===true};
    if(state.timer.running&&Number.isFinite(p.savedAt)) state.timer.remaining=Math.max(0,state.timer.remaining-Math.max(0,Date.now()-p.savedAt));
  }
}} catch { storageAvailable=false; }
let page='home', hits=[], signature='', last=performance.now(), decay=0, toastHandle, saveClock=0;
let mood='hello', moodUntil=0, timerWasRunning=state.timer.running;
let board=Array(9).fill(0), tttResult=0, tttMode='pet', tttTurn=1;
let memory=[], matched=new Set(), revealed=[], memDelay=0, memMoves=0, memWon=false;
let catchGame={running:false,left:20000,target:0,hits:0,spawn:1100};
let stopwatch={running:false,ms:0,laps:[]}, dice='?', coin='', breathStart=performance.now();
const reduced=matchMedia('(prefers-reduced-motion: reduce)').matches;
function save(){state.savedAt=Date.now();try{localStorage.setItem(STORE,JSON.stringify(state));}catch{storageAvailable=false;}updateStats();}
function toast(s){$('toast').textContent=s;$('toast').classList.add('show');clearTimeout(toastHandle);toastHandle=setTimeout(()=>$('toast').classList.remove('show'),2700);}
function updateStats(){
  $('pet-name').textContent=state.name;$('level').textContent=`Level ${1+Math.floor(state.xp/100)} · ${state.xp<100?'Sprout':state.xp<300?'Buddy':'Bloom'}`;
  $('growth-fill').style.width=state.xp%100+'%';$('next-level').textContent=`${100-state.xp%100} sparks to the next chapter`;
  for(const k of ['food','joy','energy']){$(k).textContent=Math.round(state[k]);$(k+'-bar').style.width=state[k]+'%';}
  $('wins').textContent=state.focuses;
}
function reward(xp){state.xp+=xp;state.joy=clamp(state.joy+8);mood='love';moodUntil=performance.now()+4000;save();}
function go(p){page=p;const group=['ttt','memory','catch'].includes(p)?'games':['clock','stopwatch','breathe','dice','rest'].includes(p)?'tools':p;document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('active',b.dataset.page===group));if(p==='breathe')breathStart=performance.now();}
function care(action){if(action==='feed'){state.food=clamp(state.food+15);mood='yum';toast('A tiny snack. A very big smile.');}if(action==='pat'){state.joy=clamp(state.joy+10);mood='love';}if(action==='sleep'){state.sleeping=!state.sleeping;mood=state.sleeping?'sleep':'hello';}moodUntil=performance.now()+2500;save();}
function rect(x,y,w,h,r=16,fill='#20271f'){ctx.fillStyle=fill;ctx.beginPath();ctx.roundRect(x,y,w,h,r);ctx.fill();}
function text(s,x,y,size=18,color='#eef4e8',align='center',weight=500){ctx.fillStyle=color;ctx.textAlign=align;ctx.textBaseline='middle';ctx.font=`${weight} ${size}px -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif`;ctx.fillText(s,x,y);}
function line(x1,y1,x2,y2,color='#394334',w=2){ctx.strokeStyle=color;ctx.lineWidth=w;ctx.lineCap='round';ctx.beginPath();ctx.moveTo(x1,y1);ctx.lineTo(x2,y2);ctx.stroke();}
function circle(x,y,r,color){ctx.fillStyle=color;ctx.beginPath();ctx.arc(x,y,r,0,Math.PI*2);ctx.fill();}
function hit(id,x,y,w,h,fn){hits.push({id,x,y,w,h,fn});}
function button(label,x,y,w,h,fn,primary=false,id=label){rect(x,y,w,h,18,primary?palette[state.theme]:'#1e281e');text(label,x+w/2,y+h/2,17,primary?'#1d2b19':'#dce6d5');hit(id,x,y,w,h,fn);}
function back(p='home'){button('‹  Back',168,376,130,43,()=>go(p));}
function head(title,sub=''){text(title.toUpperCase(),233,65,17,'#d5e3cb', 'center',600);if(sub)text(sub,233,93,13,'#85967a');}
function timerString(ms){const s=Math.ceil(ms/1000);return `${String(Math.floor(s/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}`;}
function pet(cx=233,cy=207,scale=1,now=performance.now()) {
  const g=ctx, time=now, color=palette[state.theme], eyes='round', reducedMotion=reduced;
  const expression=state.sleeping?'sleep':now<moodUntil?mood:'happy';
    g.save(); g.translate(cx, cy); g.scale(scale, scale);
    const bob = reducedMotion ? 0 : Math.sin(time / 850) * 2.5;
    const asleep = expression === 'sleep', loved = expression === 'love';
    const blink = !reducedMotion && time % 4600 > 4450;
    g.translate(0, bob + (loved && !reducedMotion ? Math.sin(time / 140) * 2 : 0));
    function pill(x,y,w,h,r,fill) { g.fillStyle=fill; g.beginPath(); g.roundRect(x,y,w,h,r); g.fill(); }
    // Soft ears, tiny paws and a cream belly, matching the native pet.
    pill(-74,-81,37,65,19,color); pill(37,-81,37,65,19,color);
    pill(-65,-71,19,39,10,'#f6d7cc'); pill(46,-71,19,39,10,'#f6d7cc');
    pill(-65,51,42,23,13,color); pill(23,51,42,23,13,color);
    pill(-101,-11,38,45,20,color); pill(63,-11,38,45,20,color);
    pill(-82,-57,164,118,57,color); pill(-46,14,92,42,22,'#f6f1de');
    const height=asleep||blink?5:eyes==='sleepy'?15:29;
    for(const x of [-31,31]) {
      if(loved && !asleep) {
        g.save(); g.translate(x,-13); g.scale(.56,.56); g.fillStyle='#23332b';
        g.beginPath(); g.moveTo(0,20); g.bezierCurveTo(-44,-5,-18,-33,0,-12); g.bezierCurveTo(18,-33,44,-5,0,20); g.fill(); g.restore();
      } else {
        pill(x-11,-13-height/2,22,height,eyes==='pixel'?4:Math.min(11,height/2),'#23332b');
        if(!asleep&&!blink&&eyes!=='sleepy') pill(x-7,-23,6,8,3,'#fff');
      }
    }
    pill(-62,7,23,12,6,loved?'#ff8f9e':'#f1a99f'); pill(39,7,23,12,6,loved?'#ff8f9e':'#f1a99f');
    g.strokeStyle='#23332b'; g.lineWidth=2.5; g.lineCap='round'; g.beginPath(); g.moveTo(-8,10); g.quadraticCurveTo(0,22,8,10); g.stroke();
    if(expression==='yum') { pill(-5,10,10,8,5,'#23332b'); }
    if(asleep) { g.fillStyle='#d0d7b6';g.textAlign='center';g.font='17px sans-serif';g.fillText('z',84,-57);g.font='12px sans-serif';g.fillText('z',103,-75); }
    if(loved&&!reducedMotion) { g.fillStyle='#f8b7b2';g.textAlign='center';g.font='16px sans-serif';g.fillText('♥',94,-40-Math.sin(time/300)*5); }
    g.restore();
}
function home(now){
  text(new Date().toLocaleTimeString([], {hour:'2-digit',minute:'2-digit'}),233,45,13,'#819375');
  text(state.name,233,85,23,'#e6f2dc');text(`LV ${1+Math.floor(state.xp/100)}  ·  ${state.sleeping?'DREAMING':'YOUR LITTLE COMPANION'}`,233,112,10,'#819375');pet(233,212,1,now);
  text(state.sleeping?'A little nap, a little dream.':state.food<25?'A snack would be lovely.':now<moodUntil&&mood==='love'?'Oh. You’re my favorite.':'Good to have you here.',233,294,15,'#a9bd99');
  button('Feed',83,323,94,48,()=>care('feed'));button('Pat',186,323,94,48,()=>care('pat'));button(state.sleeping?'Wake':'Nap',289,323,94,48,()=>care('sleep'));button('Explore  ↗',158,385,150,40,()=>go('apps'),true);
  hit('Pet face',115,145,236,125,()=>care('pat'));
}
function apps(){head('A little of everything');const list=[['Focus','focus'],['Arcade','games'],['Tools','tools'],['Style','style'],['My pet','home'],['Screen rest','rest']];list.forEach(([t,p],i)=>button(t,84+i%2*153,125+Math.floor(i/2)*76,145,62,()=>go(p)));back();}
function startTimer(s){state.timer={remaining:s*1000,duration:s*1000,running:true,complete:false,rewarded:false};timerWasRunning=true;save();}
function focus(){head('Make a little time',state.timer.duration<300000&&state.timer.duration>0?'QUICK TIMER':'FOCUS TOGETHER');const t=state.timer;
  ctx.strokeStyle='#24321f';ctx.lineWidth=7;ctx.beginPath();ctx.arc(233,209,86,0,Math.PI*2);ctx.stroke();
  const progress=t.duration?1-t.remaining/t.duration:0;ctx.strokeStyle=palette[state.theme];ctx.beginPath();ctx.arc(233,209,86,-Math.PI/2,-Math.PI/2+Math.PI*2*progress);ctx.stroke();
  text(timerString(t.duration?t.remaining:1500000),233,203,47,palette[state.theme], 'center',400);text(t.complete?'A LITTLE WIN.':t.running?'ONE THING AT A TIME':t.remaining?'TAKE YOUR TIME':'READY WHEN YOU ARE',233,239,10,'#91a283');
  if(t.remaining>0){button(t.running?'Pause':'Resume',103,317,124,49,()=>{t.running=!t.running;save();},true);button('Cancel',239,317,124,49,()=>{state.timer=defaults().timer;save();});}
  else{[5,25,50].forEach((m,i)=>button(m+' min',83+i*103,317,94,49,()=>startTimer(m*60),m===25));}
  button('10 sec demo',155,376,156,40,()=>startTimer(10));
}
function games(){head('Little arcade','JUST ONE MORE ROUND?');button('× ○   Tic-tac-toe',100,124,266,62,()=>go('ttt'));button('✿   Memory garden',100,201,266,62,()=>go('memory'));button('✦   Catch a spark',100,278,266,62,()=>go('catch'));back();}
const wins=[[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];
function winner(b){for(const [a,c,d]of wins)if(b[a]&&b[a]===b[c]&&b[a]===b[d])return b[a];return b.every(Boolean)?2:0;}
function solve(b,p){const w=winner(b);if(w)return w===2?0:w===p?10:-10;let best=-99;for(let i=0;i<9;i++)if(!b[i]){b[i]=p;best=Math.max(best,-solve(b,-p));b[i]=0;}return best;}
function bestMove(b,p){let move=-1,score=-99;for(const i of [4,0,2,6,8,1,3,5,7])if(!b[i]){b[i]=p;const s=-solve(b,-p);b[i]=0;if(s>score){score=s;move=i;}}return move;}
function resetTtt(){board.fill(0);tttResult=0;tttTurn=1;}
function playTtt(i){if(board[i]||tttResult)return;board[i]=tttTurn;tttResult=winner(board);if(!tttResult&&tttMode==='pet'){const m=bestMove(board,-1);if(m>=0)board[m]=-1;tttResult=winner(board);}else tttTurn=-tttTurn;if(tttResult){state.games++;reward(5);toast(tttResult===2?'A friendly draw. +5 sparks':tttResult===1?'X wins! +5 sparks':'O wins! +5 sparks');}}
function ttt(){head(tttResult?(tttResult===2?'A friendly draw':tttResult===1?'X wins!':'O wins!'):tttMode==='pet'?'Your turn · X':`${tttTurn===1?'X':'O'} to play`);text(tttMode==='pet'?`YOU vs ${state.name.toUpperCase()}`:'PASS & PLAY',233,91,11,'#829575');for(let i=0;i<9;i++){const x=119+i%3*78,y=112+Math.floor(i/3)*73;rect(x,y,72,66,14,'#1c271c');text(board[i]===1?'×':board[i]===-1?'○':'·',x+36,y+33,39,board[i]===1?palette[state.theme]:'#96b8d9');hit('Square '+(i+1),x,y,72,66,()=>playTtt(i));}button('Again',103,338,124,39,resetTtt);button(tttMode==='pet'?'2 players':'vs Pebble',239,338,124,39,()=>{tttMode=tttMode==='pet'?'friend':'pet';resetTtt();});button('‹ Arcade',168,387,130,37,()=>go('games'));}
function newMemory(){memory=[0,0,1,1,2,2,3,3];for(let i=7;i;i--){const j=Math.floor(Math.random()*(i+1));[memory[i],memory[j]]=[memory[j],memory[i]];}matched=new Set();revealed=[];memDelay=0;memMoves=0;memWon=false;}
function flip(i){if(memWon||memDelay||matched.has(i)||revealed.includes(i))return;revealed.push(i);if(revealed.length===2){memMoves++;if(memory[revealed[0]]===memory[i]){revealed.forEach(n=>matched.add(n));revealed=[];if(matched.size===8){memWon=true;state.games++;reward(15);toast('Your garden is blooming. +15 sparks');}}else memDelay=900;}}
function memoryScreen(){head(memWon?'A blooming good memory':'Memory garden',`${matched.size/2} / 4 PAIRS  ·  ${memMoves} TURNS`);for(let i=0;i<8;i++){const x=85+i%4*76,y=143+Math.floor(i/4)*85,show=matched.has(i)||revealed.includes(i);rect(x,y,68,75,14,matched.has(i)?'#304527':'#1e2a1d');text(show?['✿','☀','◆','✦'][memory[i]]:'?',x+34,y+37,32,show?['#c7f568','#ffcb6b','#9ec5ff','#e4abdf'][memory[i]]:'#657f58');hit('Memory tile '+(i+1),x,y,68,75,()=>flip(i));}button('New garden',143,321,180,43,newMemory);back('games');}
function catchStart(){catchGame={running:true,left:20000,target:Math.floor(Math.random()*6),hits:0,spawn:1100};}
function catchHit(i){if(!catchGame.running||catchGame.target!==i)return;catchGame.hits++;catchGame.target=(i+1+Math.floor(Math.random()*5))%6;catchGame.spawn=1100;}
function catchScreen(){head('Catch a spark',`${Math.ceil(catchGame.left/1000)} SEC  ·  ${catchGame.hits} SPARKS`);for(let i=0;i<6;i++){const x=116+i%3*80,y=132+Math.floor(i/3)*80;rect(x,y,72,70,20,'#1d291d');if(catchGame.running&&catchGame.target===i)text('✦',x+36,y+35,42,palette[state.theme]);else circle(x+36,y+35,3,'#48613c');hit('Catch spot '+(i+1),x,y,72,70,()=>catchHit(i));}if(!catchGame.running)button(catchGame.left===0?'Play again':'Let’s play',143,313,180,48,catchStart,true);else text('Tap the spark!',233,338,16,'#91a582');back('games');}
function tools(){head('Everyday, a little better');const list=[['Clock','clock'],['Stopwatch','stopwatch'],['Breathe','breathe'],['Dice & coin','dice'],['Timer','focus'],['Screen rest','rest']];list.forEach(([t,p],i)=>button(t,84+i%2*153,125+Math.floor(i/2)*76,145,62,()=>go(p)));back();}
function clock(){head('Right here. Right now.');const d=new Date();text(d.toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}),233,191,54,palette[state.theme], 'center',400);text(d.toLocaleDateString([],{weekday:'long',month:'short',day:'numeric'}),233,242,16,'#95ab86');text('TAKE THE DAY AT YOUR PACE',233,291,10,'#718866');back('tools');}
function stopwatchScreen(){head('A moment, measured');text(timerString(Math.floor(stopwatch.ms/1000)*1000)+'.'+Math.floor(stopwatch.ms/100)%10,233,170,43,palette[state.theme]);text(stopwatch.laps.length?'Last lap  '+timerString(stopwatch.laps.at(-1)):'Make it count.',233,216,14,'#90a581');button(stopwatch.running?'Pause':'Start',103,270,124,52,()=>stopwatch.running=!stopwatch.running,true);button(stopwatch.running?'Lap':'Reset',239,270,124,52,()=>{if(stopwatch.running)stopwatch.laps.push(stopwatch.ms);else stopwatch={running:false,ms:0,laps:[]};});back('tools');}
function breathe(now){head('One quiet moment');const phase=(now-breathStart)%14000,expand=phase<4000?phase/4000:phase<8000?1:1-(phase-8000)/6000;circle(233,220,50+expand*35,'#23351c');circle(233,220,37+expand*25,'#4c6935');circle(233,220,22+expand*15,palette[state.theme]);text(phase<4000?'Breathe in':phase<8000?'Hold gently':'Breathe out',233,331,21,'#bdcea9');text('4 IN · 4 HOLD · 6 OUT',233,96,11,'#819775');back('tools');}
function diceScreen(){head('Leave it to chance');text(String(dice),168,197,65,palette[state.theme]);text(coin||'○',297,197,coin?22:65,'#afc49d');button('Roll die',103,280,124,55,()=>dice=1+Math.floor(Math.random()*6),true);button('Flip coin',239,280,124,55,()=>coin=Math.random()<.5?'Heads':'Tails');back('tools');}
function rename(){const d=$('name-dialog');$('name-input').value=state.name;d.showModal();$('name-input').focus();}
function style(){head('A little more you');pet(233,185,.62);text('PICK A PERSONALITY COLOR',233,261,10,'#859a77');palette.forEach((c,i)=>{circle(152+i*54,296,17,c);if(state.theme===i){ctx.strokeStyle='#fff';ctx.lineWidth=2;ctx.beginPath();ctx.arc(152+i*54,296,22,0,Math.PI*2);ctx.stroke();}hit(['Moss','Sky','Peach','Lilac'][i]+' pet',129+i*54,273,46,46,()=>{state.theme=i;save();});});button('Rename',103,337,124,43,rename);button('Portrait ↗',239,337,124,43,exportPortrait);button('‹ Home',168,390,130,35,()=>go('home'));}
function rest(){text('RESTING',233,233,11,'#3b5032');hit('Wake screen',50,50,366,366,()=>go('home'));}
function syncControls(){const sig=hits.map(h=>[h.id,h.x,h.y,h.w,h.h].join(':')).join('|');if(sig===signature)return;signature=sig;const parent=$('touch-controls');parent.replaceChildren();hits.forEach((h,i)=>{const b=document.createElement('button');b.className='touch-target';b.setAttribute('aria-label',h.id);Object.assign(b.style,{left:h.x/466*100+'%',top:h.y/466*100+'%',width:h.w/466*100+'%',height:h.h/466*100+'%'});b.onclick=()=>{if(hits[i])hits[i].fn();save();};parent.append(b);});}
function tick(now){let dt=Math.max(0,now-last);last=now;decay+=dt;saveClock+=dt;
  if(decay>=60000){const m=Math.min(360,Math.floor(decay/60000));decay%=60000;state.food=clamp(state.food-m);state.joy=clamp(state.joy-m);state.energy=clamp(state.energy+(state.sleeping?3*m:-m));updateStats();}
  const t=state.timer;if(t.running){t.remaining=Math.max(0,t.remaining-dt);if(t.remaining===0){t.running=false;t.complete=true;if(!t.rewarded){t.rewarded=true;if(t.duration>=300000){state.focuses++;reward(25);toast('Time well spent. +25 sparks for '+state.name);}else{toast('Timer complete. A little win.');save();}}}}
  if(memDelay){memDelay=Math.max(0,memDelay-dt);if(!memDelay)revealed=[];}
  if(catchGame.running){catchGame.left=Math.max(0,catchGame.left-dt);catchGame.spawn-=dt;if(!catchGame.left){catchGame.running=false;state.games++;reward(catchGame.hits);toast(catchGame.hits+' sparks caught!');}else if(catchGame.spawn<=0){catchGame.target=Math.floor(Math.random()*6);catchGame.spawn=1100;}}
  if(stopwatch.running)stopwatch.ms+=dt;if(saveClock>15000){saveClock=0;save();}
  hits=[];ctx.clearRect(0,0,466,466);ctx.fillStyle='#090e09';ctx.fillRect(0,0,466,466);
  ({home,apps,focus,games,ttt,memory:memoryScreen,catch:catchScreen,tools,clock,stopwatch:stopwatchScreen,breathe,dice:diceScreen,style,rest}[page]||home)(now);syncControls();
  $('device-caption').textContent=state.sleeping?`${state.name} is having a little dream.`:state.timer.running?`${state.name} is focusing with you.`:`${state.name} is happy to see you.`;
  requestAnimationFrame(tick);
}
function download(blob,name){const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),3000);}
function exportPortrait(){const c=document.createElement('canvas');c.width=1080;c.height=1350;const g=c.getContext('2d');g.fillStyle='#edf2e3';g.fillRect(0,0,1080,1350);g.fillStyle='#344a2c';g.textAlign='center';g.font='24px sans-serif';g.fillText('A LITTLE LIFE IN MY POCKET',540,130);g.font='80px sans-serif';g.fillText(state.name,540,250);g.save();g.beginPath();g.arc(540,630,310,0,Math.PI*2);g.clip();g.fillStyle='#090e09';g.fillRect(230,320,620,620);g.fillStyle=palette[state.theme];for(const x of [440,640]){g.beginPath();g.roundRect(x-38,520,76,state.sleeping?12:115,38);g.fill();}g.strokeStyle=palette[state.theme];g.lineWidth=8;g.beginPath();g.arc(540,680,26,0,Math.PI);g.stroke();g.restore();g.fillStyle='#506346';g.font='30px sans-serif';g.fillText(`Level ${1+Math.floor(state.xp/100)}  ·  ${state.focuses} focus sessions together`,540,1040);g.font='27px sans-serif';g.fillText('Small screen. Big personality.',540,1130);g.font='20px sans-serif';g.fillText('PEBBLE microOS · CONCEPT PREVIEW',540,1250);c.toBlob(blob=>{if(blob){download(blob,'My-Pebble.png');toast('Portrait saved. Make it your own little story.');}},'image/png');}
function finish(name){const colors={moss:'#65765b',milk:'#dedbd0',ink:'#343a36'};state.finish=name;document.documentElement.style.setProperty('--shell',colors[name]);$('finish-name').textContent=name.toUpperCase();document.querySelectorAll('[data-finish]').forEach(b=>b.classList.toggle('selected',b.dataset.finish===name));save();}
document.querySelectorAll('[data-page]').forEach(b=>b.onclick=()=>go(b.dataset.page));document.querySelectorAll('[data-finish]').forEach(b=>b.onclick=()=>finish(b.dataset.finish));
$('passport').onclick=exportPortrait;$('backup').onclick=()=>download(new Blob([JSON.stringify(state,null,2)],{type:'application/json'}),'Pebble-pet-backup.json');
$('name-dialog').addEventListener('close',()=>{if($('name-dialog').returnValue==='save'){const n=$('name-input').value.trim();if(n){state.name=n.slice(0,16);save();toast('Hello, '+state.name+'.');}}});
document.addEventListener('keydown',e=>{if(e.key==='Escape'&&!$('name-dialog').open)go('home');});
addEventListener('pagehide',save);newMemory();finish(state.finish);updateStats();requestAnimationFrame(tick);if(!storageAvailable)toast('Browser storage unavailable. This pet lives in this session.');
