// Lab in EDAN25 http://cs.lth.se/edan25/labs/

import java.util.Iterator;
import java.util.ListIterator;
import java.util.LinkedList;
import java.util.BitSet;
import java.io.*;
import java.util.concurrent.Semaphore;


class Random {
	int	w;
	int	z;

	public Random(int seed) {
		w = seed + 1;
		z = seed * seed + seed + 2;
	}

	int nextInt() {
		z = 36969 * (z & 65535) + (z >> 16);
		w = 18000 * (w & 65535) + (w >> 16);

		return (z << 16) + w;
	}
}

class Vertex {
	int			index;
	boolean			listed;
	LinkedList<Vertex>	pred;
	LinkedList<Vertex>	succ;
	BitSet			in;
	BitSet			out;
	BitSet			use;
	BitSet			def;

	Vertex(int i) {
		index	= i;
		pred	= new LinkedList<Vertex>();
		succ	= new LinkedList<Vertex>();
		in	= new BitSet();
		out	= new BitSet();
		use	= new BitSet();
		def	= new BitSet();
	}

	void computeIn(LinkedList<Vertex> worklist) {
		int			i;
		BitSet			old;
		BitSet			ne;
		Vertex			v;
		ListIterator<Vertex>	iter;
		iter = succ.listIterator();
		while (iter.hasNext()) {
			v = iter.next();
			out.or(v.in);
		}
		old = in;
		in = new BitSet();
		in.or(out);
		in.andNot(def);
		in.or(use);
		if (!in.equals(old)) {
			iter = pred.listIterator();
			while (iter.hasNext()) {
				v = iter.next();
				if (!v.listed) {
					worklist.addLast(v);
					v.listed = true;
				}
			}

		}
	}

	public void print()	{
		try {
			PrintWriter writer = new PrintWriter(new FileOutputStream(new File("new.txt"), true));
			int	i;

			writer.print("use[" + index + "] = { ");
			for (i = 0; i < use.size(); ++i)
				if (use.get(i))
					writer.print("" + i + " ");
			writer.println("}");
			writer.print("def[" + index + "] = { ");
			for (i = 0; i < def.size(); ++i)
				if (def.get(i))
					writer.print("" + i + " ");
			writer.println("}\n");

			writer.print("in[" + index + "] = { ");
			for (i = 0; i < in.size(); ++i)
				if (in.get(i))
					writer.print("" + i + " ");
			writer.println("}");

			writer.print("out[" + index + "] = { ");
			for (i = 0; i < out.size(); ++i)
				if (out.get(i))
					writer.print("" + i + " ");
			writer.println("}\n");
			writer.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}

class Dataflow {

	public static void connect(Vertex pred, Vertex succ) {
		pred.succ.addLast(succ);
		succ.pred.addLast(pred);
	}

	public static void generateCFG(Vertex vertex[], int maxsucc, Random r) {
		int	i;
		int	j;
		int	k;
		int	s;	// number of successors of a vertex.

		System.out.println("generating CFG...");

		connect(vertex[0], vertex[1]);
		connect(vertex[0], vertex[2]);

		for (i = 2; i < vertex.length; ++i) {
			s = (r.nextInt() % maxsucc) + 1;
			for (j = 0; j < s; ++j) {
				k = Math.abs(r.nextInt()) % vertex.length;
				connect(vertex[i], vertex[k]);
			}
		}
	}

	public static void generateUseDef(
	    Vertex	vertex[],
	    int	nsym,
	    int	nactive,
	    Random	r) {
		int	i;
		int	j;
		int	sym;

		System.out.println("generating usedefs...");

		for (i = 0; i < vertex.length; ++i) {
			for (j = 0; j < nactive; ++j) {
				sym = Math.abs(r.nextInt()) % nsym;

				if (j % 4 != 0) {
					if (!vertex[i].def.get(sym))
						vertex[i].use.set(sym);
				} else {
					if (!vertex[i].use.get(sym))
						vertex[i].def.set(sym);
				}
			}
		}
	}



	public static void main(String[] args) {
		int	i;
		int	nsym;
		int	nvertex;
		int	maxsucc;
		int	nactive;
		int	nthread;
		boolean	print;
		Vertex	vertex[];
		Random	r;

		r = new Random(1);

		nsym = Integer.parseInt(args[0]);
		nvertex = Integer.parseInt(args[1]);
		maxsucc = Integer.parseInt(args[2]);
		nactive = Integer.parseInt(args[3]);
		nthread = Integer.parseInt(args[4]);
		print = Integer.parseInt(args[5]) != 0;

		System.out.println("nsym = " + nsym);
		System.out.println("nvertex = " + nvertex);
		System.out.println("maxsucc = " + maxsucc);
		System.out.println("nactive = " + nactive);

		vertex = new Vertex[nvertex];

		for (i = 0; i < vertex.length; ++i)
			vertex[i] = new Vertex(i);

		generateCFG(vertex, maxsucc, r);
		generateUseDef(vertex, nsym, nactive, r);
		liveness(vertex, nthread);

		if (print)
			for (i = 0; i < vertex.length; ++i)
				vertex[i].print();
	}

	public static void liveness(Vertex vertex[], int nthread) {
		Vertex			u;
		LinkedList<Vertex>	worklist;
		long			begin;
		long			end;


		begin = System.nanoTime();
		worklist = new LinkedList<Vertex>();

		int partSize = vertex.length / nthread;
		LinkedList<livenessThread> t = new LinkedList<livenessThread>();

		for (int i = 0; i < nthread; i++) {
			worklist = new LinkedList<Vertex>();
			for (int j = i * partSize; j < (i + 1) * partSize; ++j) {
				worklist.addLast(vertex[j]);
				vertex[i].listed = true;
			}
			t.add(new livenessThread(worklist));
		}

		for (livenessThread livenessThread : t) {
			livenessThread.start();
		}
		try {
			for (livenessThread livenessThread : t) {
				livenessThread.join();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

		end = System.nanoTime();

		System.out.println("T = " + (end - begin) / 1e9 + " s");
	}
}
class livenessThread extends Thread {

	Vertex			u;
	int			i;
	LinkedList<Vertex>	worklist;

	public livenessThread(LinkedList<Vertex> worklist) {
		this.worklist = worklist;
	}

	public void run() {

		System.out.println("computing liveness...");
		System.out.println("Worklist size: " + worklist.size());
		while (!worklist.isEmpty()) {
			u = worklist.remove();
			u.listed = false;
			u.computeIn(worklist);
		}

	}
}