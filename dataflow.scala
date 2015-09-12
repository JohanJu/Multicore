// Lab in EDAN25 http://cs.lth.se/edan25/labs/

import scala.actors._
import java.util.BitSet;
import java.util.concurrent.Semaphore;
// LAB 2: some case classes but you need additional ones too.

case class Start();
case class Stop();
case class Ready();
case class Go();
case class Done();
case class Same();
case class Diff();
case class Change(in: BitSet);

class Random(seed: Int) {
        var w = seed + 1;
        var z = seed * seed + seed + 2;

        def nextInt() =
        {
                z = 36969 * (z & 65535) + (z >> 16);
                w = 18000 * (w & 65535) + (w >> 16);

                (z << 16) + w;
        }
}

class Controller(val cfg: Array[Vertex]) extends Actor {
  var started = 0;
  val begin   = System.currentTimeMillis();
  var sa           	= 0;
  var di           	= 0;

  def act() {
    react {
      case Ready() => {
        started += 1;
        // println("controller has seen " + started);
        if (started == cfg.length) {
        	// println("all started " + started);
        	sa = 0;
        	di = 0;
          for (u <- cfg){
          	// print("go ");
            u ! new Go;
            }
        }
        act();
      }
      case Same() => {
      	// println("Same " + (sa+di));
        sa = sa + 1;
        if(sa == cfg.length){
        	println("all done ");
        	for (u <- cfg){
            u ! new Stop;
            }
            this ! new Stop;
            Driver.mutex.release();

        }else if((sa+di) == cfg.length){
        	started = started -1;
        	this ! new Ready;

        	// for (u <- cfg){
         //    u ! new Start;
         //    }
        }
        act();
      }
      case Diff() => {
      	// println("Diff " + (sa+di));
        di = di + 1;
        if((sa+di) == cfg.length){
        	started = started -1;
        	this ! new Ready;
        	// for (u <- cfg){
         //    u ! new Start;
         //    }
        }
        act();
      }
      case Stop()  => { 

      }

    }
  }
}

class Vertex(val index: Int, s: Int, val controller: Controller) extends Actor {
  var pred: List[Vertex] = List();
  var succ: List[Vertex] = List();
  val uses               = new BitSet(s);
  val defs               = new BitSet(s);
  var in                 = new BitSet(s);
  var out                = new BitSet(s);
  var old               = new BitSet(s);
  var sn1            	= 0;
  var sn2            	= 0;
  val qwe 				= 12;
  def act() {
    react {
      case Start() => {
      	// if (index == qwe) println("start "+index+" sn "+sn1);
      	sn2 = sn1;
        controller ! new Ready;
        act(); 
      }
      case Change(in)=> {
      	// if (index == qwe) println("Change "+index+" sn "+sn2);
      	out.or(in);
        sn2=sn2-1;
        if(sn2==0){
          this ! new Done();
        }
      	act();
      }
      case Go() => {
      	// if (index == qwe) println("go "+index);
      	// sn2 = sn1;
      	for (v <- pred){
      		v ! new Change(in);
      	}
      	if(sn1==0){
          this ! new Done();
        }
        act();
      }
      case Done()  => {
      	// println("done "+index);
      	sn2 = sn1; 
        old = in;
    	in = new BitSet();
   	 	in.or(out);
    	in.andNot(defs);
   		in.or(uses);
   		if (in.equals(old)) {
      		controller ! new Same;
   		}else{
   			controller ! new Diff;
   		}
   		act();
      }
      case Stop()  => { 

      }
    }
  }

  def connect(that: Vertex)
  {
    //println(this.index + "->" + that.index);
    this.succ = that :: this.succ;
    that.pred = this :: that.pred;
    this.sn1 +=1;
  }

  def printSet(name: String, index: Int, set: BitSet) {
    System.out.print(name + "[" + index + "] = { ");
    for (i <- 0 until set.size)
      if (set.get(i))
        System.out.print("" + i + " ");
    println("}");
  }

  def print = {
    printSet("use", index, uses);
    printSet("def", index, defs);
    printSet("in", index, in);
    println("");
  }
}

object Driver {
  val rand    = new Random(1);
  var nactive = 0;
  var nsym    = 0;
  var nvertex = 0;
  var maxsucc = 0;
  var mutex = new Semaphore(0);

  def makeCFG(cfg: Array[Vertex]) {

    cfg(0).connect(cfg(1));
    cfg(0).connect(cfg(2));

    for (i <- 2 until cfg.length) {
      val p = cfg(i);
      val s = (rand.nextInt() % maxsucc) + 1;

      for (j <- 0 until s) {
        val k = cfg((rand.nextInt() % cfg.length).abs);
        p.connect(k);
      }
    }
  }

  def makeUseDef(cfg: Array[Vertex]) {
    for (i <- 0 until cfg.length) {
      for (j <- 0 until nactive) {
        val s = (rand.nextInt() % nsym).abs;
        if (j % 4 != 0) {
          if (!cfg(i).defs.get(s))
            cfg(i).uses.set(s);
        } else {
          if (!cfg(i).uses.get(s))
            cfg(i).defs.set(s);
        }
      }
    }
  }

  def main(args: Array[String]) {
    nsym           = args(0).toInt;
    nvertex        = args(1).toInt;
    maxsucc        = args(2).toInt;
    nactive        = args(3).toInt;
    val print      = args(4).toInt;
    val cfg        = new Array[Vertex](nvertex);
    val controller = new Controller(cfg);


    controller.start;

    println("generating CFG...");
    for (i <- 0 until nvertex)
      cfg(i) = new Vertex(i, nsym, controller);

    makeCFG(cfg);
    println("generating usedefs...");
    makeUseDef(cfg);

    println("starting " + nvertex + " actors...");

    for (i <- 0 until nvertex)
      cfg(i).start;

    for (i <- 0 until nvertex)
      cfg(i) ! new Start;

      mutex.acquire();

    if (print != 0)
      for (i <- 0 until nvertex)
        cfg(i).print;
  }
}
