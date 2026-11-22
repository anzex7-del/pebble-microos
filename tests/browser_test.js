function assert(condition,message){if(!condition)throw new Error(message);}
function step(ms){perf+=ms;tick(perf);}
assert(state.name==='Milo','default name');
for(const p of ['home','apps','focus','games','ttt','memory','catch','tools','clock','stopwatch','breathe','dice','style','rest']){go(p);step(16);assert(hits.length>0,'no controls for '+p);}
go('home');step(16);hits.find(h=>h.id==='Feed').fn();assert(state.food===95,'feed');care('feed');assert(state.food===100,'clamped food');
startTimer(300);step(100000);assert(state.timer.remaining===200000,'timer elapsed');state.timer.running=false;step(1000);assert(state.timer.remaining===200000,'timer pause');state.timer.running=true;step(200001);assert(state.focuses===1&&state.xp===25,'timer reward');step(1000);assert(state.focuses===1,'reward repeated');startTimer(10);step(10001);assert(state.focuses===1,'demo must not award focus');
let paths=0;function explore(b){let r=winner(b);if(r){assert(r!==1,'AI lost');paths++;return;}for(let i=0;i<9;i++)if(!b[i]){let c=b.slice();c[i]=1;let w=winner(c);if(!w){c[bestMove(c,-1)]=-1;}explore(c);}}
explore(Array(9).fill(0));assert(paths===635,'ttt exhaustive path count');
resetTtt();playTtt(0);assert(board.filter(Boolean).length===2,'AI response');const b=board.slice();playTtt(0);assert(String(b)===String(board),'occupied square');
newMemory();for(let i=0;i<8;i++)for(let j=i+1;j<8;j++)if(memory[i]===memory[j]){flip(i);flip(i);assert(revealed.length===1,'same card twice');flip(j);}assert(memWon&&matched.size===8,'memory completion');let oldxp=state.xp;flip(0);assert(state.xp===oldxp,'memory duplicate reward');
newMemory();const mismatch=memory.findIndex(n=>n!==memory[0]);flip(0);flip(mismatch);assert(memDelay===900,'mismatch delay');step(901);assert(revealed.length===0,'mismatch cleared');
catchStart();let target=catchGame.target;catchHit(target);catchHit(target);assert(catchGame.hits===1,'catch duplicate tap');step(20001);assert(!catchGame.running,'catch completion');
stopwatch.running=true;step(5000);assert(stopwatch.ms===5000,'stopwatch');stopwatch.running=false;step(1000);assert(stopwatch.ms===5000,'stopwatch pause');
save();assert(JSON.parse(saved.get(STORE)).focuses===1,'persistent focus count');
exportPortrait();
