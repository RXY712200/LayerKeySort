//! Stable occurrence adapter around unchanged upstream classical priorities.
use order_maintenance::{MaintainedOrd,tag_range,list_range};
use std::{alloc::{GlobalAlloc,Layout,System},cell::Cell,cmp::Ordering,env,fs,time::Instant};
thread_local! { static ACTIVE:Cell<bool>=const{Cell::new(false)}; static LIVE:Cell<usize>=const{Cell::new(0)}; static PEAK:Cell<usize>=const{Cell::new(0)}; static CALLS:Cell<usize>=const{Cell::new(0)}; }
#[cfg(feature="diagnostic")]
struct Alloc;
fn add(n:usize) { LIVE.with(|l|{let v=l.get()+n;l.set(v);PEAK.with(|p|p.set(p.get().max(v)));});CALLS.with(|c|c.set(c.get()+1)); }
#[cfg(feature="diagnostic")]
unsafe impl GlobalAlloc for Alloc {
    unsafe fn alloc(&self,l:Layout)->*mut u8 {let p=System.alloc(l);if !p.is_null()&&ACTIVE.with(Cell::get){add(l.size());}p}
    unsafe fn dealloc(&self,p:*mut u8,l:Layout) {if ACTIVE.with(Cell::get){LIVE.with(|x|x.set(x.get().checked_sub(l.size()).expect("accounting underflow")));}System.dealloc(p,l)}
    unsafe fn realloc(&self,p:*mut u8,l:Layout,n:usize)->*mut u8 {let q=System.realloc(p,l,n);if !q.is_null()&&ACTIVE.with(Cell::get){LIVE.with(|x|x.set(x.get()-l.size()));add(n);}q}
}
#[cfg(feature="diagnostic")]
#[global_allocator] static ALLOC:Alloc=Alloc;
#[inline] fn active(value:bool){#[cfg(feature="diagnostic")] ACTIVE.with(|x|x.set(value)); #[cfg(not(feature="diagnostic"))] let _=value;}
const NONE:usize=usize::MAX;
struct Node<P>{prio:P,prev:usize,next:usize,payload:usize}
struct Seq<P>{base:P,nodes:Vec<Option<Box<Node<P>>>>,first:usize,last:usize}
impl<P:MaintainedOrd> Seq<P> {
    fn insert(&mut self,id:usize,anchor:usize,after:bool) {
        let prev=if anchor==NONE {self.last} else if after {anchor} else {self.nodes[anchor].as_ref().unwrap().prev};
        let prio=if prev==NONE {self.base.insert()} else {self.nodes[prev].as_ref().unwrap().prio.insert()};
        let next=if prev==NONE{self.first}else{self.nodes[prev].as_ref().unwrap().next};
        active(true);let node=Box::new(Node{prio,prev,next,payload:id%64});active(false);
        self.nodes[id]=Some(node);self.link(id,prev,next);
    }
    fn link(&mut self,id:usize,prev:usize,next:usize){if prev==NONE{self.first=id}else{self.nodes[prev].as_mut().unwrap().next=id;}if next==NONE{self.last=id}else{self.nodes[next].as_mut().unwrap().prev=id;}}
    fn unlink(&mut self,id:usize){let x=self.nodes[id].as_ref().unwrap();let(p,n)=(x.prev,x.next);if p==NONE{self.first=n}else{self.nodes[p].as_mut().unwrap().next=n;}if n==NONE{self.last=p}else{self.nodes[n].as_mut().unwrap().prev=p;}}
    fn remove(&mut self,id:usize){self.unlink(id);active(true);drop(self.nodes[id].take());active(false);}
    fn mov(&mut self,id:usize,a:usize,after:bool){
        let node=self.nodes[id].as_ref().unwrap();if id==a||(after&&node.prev==a)||(!after&&node.next==a){return;}
        let prev=if after{a}else{self.nodes[a].as_ref().unwrap().prev};
        active(true);let p=if prev==NONE{self.base.insert()}else{self.nodes[prev].as_ref().unwrap().prio.insert()};active(false);
        self.unlink(id);let next=if prev==NONE{self.first}else{self.nodes[prev].as_ref().unwrap().next};
        let x=self.nodes[id].as_mut().unwrap();x.prev=prev;x.next=next;active(true);x.prio=p;active(false);self.link(id,prev,next);
    }
    fn verify(&self,model:&[usize]){let mut id=self.first;for &expected in model{assert_eq!(id,expected);let x=self.nodes[id].as_ref().unwrap();assert_eq!(x.payload,id%64);id=x.next;}assert_eq!(id,NONE);id=self.last;for &expected in model.iter().rev(){assert_eq!(id,expected);id=self.nodes[id].as_ref().unwrap().prev;}assert_eq!(id,NONE);for pair in model.windows(2){assert!(self.nodes[pair[0]].as_ref().unwrap().prio<self.nodes[pair[1]].as_ref().unwrap().prio);}}
}
fn run<P:MaintainedOrd>(path:&str,diag:bool){
    let text=fs::read_to_string(path).unwrap();let mut t=text.split_whitespace();let n:usize=t.next().unwrap().parse().unwrap();let count:usize=t.next().unwrap().parse().unwrap();let last:usize=t.next().unwrap().parse().unwrap();assert!(n<=100000&&count<=20000);
    let mut ops=Vec::with_capacity(count);for _ in 0..count{let k=t.next().unwrap().as_bytes()[0];let a=t.next().unwrap().parse::<usize>().unwrap();let b=t.next().unwrap().parse::<usize>().unwrap();let after=t.next().unwrap()=="1";let cmp=t.next().unwrap().parse::<i32>().unwrap();ops.push((k,a,b,after,cmp));}
    let expected:Vec<usize>=(0..last).map(|_|t.next().unwrap().parse().unwrap()).collect();assert!(t.next().is_none());
    let mut nodes=Vec::with_capacity(n+count+1);nodes.resize_with(n+count+1,||None);
    active(true);let base=P::new();active(false);let mut s=Seq{base,nodes,first:NONE,last:NONE};
    // Backend requests are tracked only while manipulating the representation.
    for id in 0..n{active(true);s.insert(id,NONE,true);active(false);}
    let mut model:Vec<usize>=(0..n).collect();s.verify(&model);
    let identities:Vec<usize>=s.nodes.iter().map(|x|x.as_ref().map_or(0,|v|&**v as *const _ as usize)).collect();
    let start=Instant::now();
    for (step,&(k,a,b,after,cmp)) in ops.iter().enumerate(){
        match k{b'C'=>{let o=s.nodes[a].as_ref().unwrap().prio.partial_cmp(&s.nodes[b].as_ref().unwrap().prio).unwrap();assert_eq!(match o{Ordering::Less=>-1,Ordering::Equal=>0,Ordering::Greater=>1},cmp);},b'I'=>{active(true);s.insert(a,b,after);active(false);},b'R'=>s.remove(a),b'M'=>s.mov(a,b,after),_=>panic!("bad operation")}
        if diag{match k{b'R'=>{let i=model.iter().position(|&x|x==a).unwrap();model.remove(i);},b'M' if a!=b=>{let i=model.iter().position(|&x|x==a).unwrap();model.remove(i);let j=model.iter().position(|&x|x==b).unwrap();model.insert(j+after as usize,a);},b'I'=>{let j=model.iter().position(|&x|x==b).unwrap();model.insert(j+after as usize,a);},_=>{}}if step%128==0{s.verify(&model);}}
    }
    let elapsed=start.elapsed().as_secs_f64();s.verify(&expected);
    for &id in &expected{if id<n{assert_eq!(identities[id],&**s.nodes[id].as_ref().unwrap() as *const _ as usize);}}
    let (live,peak,calls)=(LIVE.with(Cell::get),PEAK.with(Cell::get),CALLS.with(Cell::get));
    for id in 0..s.nodes.len(){if s.nodes[id].is_some(){s.remove(id);}}
    active(true);drop(s.base);active(false);let remaining=LIVE.with(Cell::get);assert_eq!(remaining,0);
    println!("{elapsed:.9},{live},{peak},{calls},0,{remaining},0,0,0,0,0,0,0,0,0");
}
fn main(){let a:Vec<String>=env::args().collect();assert_eq!(a.len(),4);match a[1].as_str(){"tag"=>run::<tag_range::Priority>(&a[2],a[3]=="1"),"list_range"=>run::<list_range::Priority>(&a[2],a[3]=="1"),_=>panic!("unknown backend")}}
