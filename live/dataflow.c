#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include "dataflow.h"
#include "error.h"
#include "list.h"
#include "set.h"
#define NT (4)

typedef struct vertex_t	vertex_t;
typedef struct task_t	task_t;

/* cfg_t: a control flow graph. */
struct cfg_t {
	size_t			nvertex;	/* number of vertices		*/
	size_t			nsymbol;	/* width of bitvectors		*/
	vertex_t*		vertex;		/* array of vertex		*/
};

/* vertex_t: a control flow graph vertex. */
struct vertex_t {
	size_t			index;		/* can be used for debugging	*/
	set_t*			set[NSETS];	/* live in from this vertex	*/
	set_t*			prev;		/* alternating with set[IN]	*/
	size_t			nsucc;		/* number of successor vertices */
	vertex_t**		succ;		/* successor vertices 		*/
	list_t*			pred;		/* predecessor vertices		*/
	bool			listed;		/* on worklist			*/
	bool 			inUse;
	pthread_mutex_t mutexIn;	

};

static void clean_vertex(vertex_t* v);
static void init_vertex(vertex_t* v, size_t index, size_t nsymbol, size_t max_succ);

cfg_t* new_cfg(size_t nvertex, size_t nsymbol, size_t max_succ){
	size_t		i;
	cfg_t*		cfg;

	cfg = calloc(1, sizeof(cfg_t));
	if (cfg == NULL)
		error("out of memory");

	cfg->nvertex = nvertex;
	cfg->nsymbol = nsymbol;

	cfg->vertex = calloc(nvertex, sizeof(vertex_t));
	if (cfg->vertex == NULL)
		error("out of memory");

	for (i = 0; i < nvertex; i += 1)
		init_vertex(&cfg->vertex[i], i, nsymbol, max_succ);

	return cfg;}

static void clean_vertex(vertex_t* v){
	int		i;

	for (i = 0; i < NSETS; i += 1)
		free_set(v->set[i]);
	free_set(v->prev);
	free(v->succ);
	free_list(&v->pred);}

static void init_vertex(vertex_t* v, size_t index, size_t nsymbol, size_t max_succ){
	int		i;

	v->index	= index;
	v->succ		= calloc(max_succ, sizeof(vertex_t*));
	if (v->succ == NULL)
		error("out of memory");
	
	for (i = 0; i < NSETS; i += 1)
		v->set[i] = new_set(nsymbol);

	v->prev = new_set(nsymbol);}

void free_cfg(cfg_t* cfg){
	size_t		i;

	for (i = 0; i < cfg->nvertex; i += 1)
		clean_vertex(&cfg->vertex[i]);
	free(cfg->vertex);
	free(cfg);}

void connect(cfg_t* cfg, size_t pred, size_t succ){
	vertex_t*	u;
	vertex_t*	v;

	u = &cfg->vertex[pred];
	v = &cfg->vertex[succ];

	u->succ[u->nsucc++ ] = v;
	insert_last(&v->pred, u);}

bool testbit(cfg_t* cfg, size_t v, set_type_t type, size_t index){
	return test(cfg->vertex[v].set[type], index);}

void setbit(cfg_t* cfg, size_t v, set_type_t type, size_t index){
	set(cfg->vertex[v].set[type], index);}

list_t*			worklist[NT];
pthread_mutex_t mutexWork[NT];
pthread_mutex_t mutexEnd;
int end = 1;

void* lthread(void* arg) {
	vertex_t*	u;
	vertex_t*	us;
	vertex_t*	v;
	set_t*		prev;
	size_t		i;
	size_t		nbr;
	list_t*		p;
	list_t*		h;

	nbr = (size_t)arg;

	int done = 1;
	while (done) {

		pthread_mutex_lock(&mutexWork[nbr]);
		u = remove_first(&worklist[nbr]);
		pthread_mutex_unlock(&mutexWork[nbr]);

		while (1) {
			if(u  == NULL){
				pthread_mutex_lock(&mutexEnd);
				end&=~(1<<nbr);
				pthread_mutex_unlock(&mutexEnd);
				break;
			}else{
				pthread_mutex_lock(&mutexEnd);
				end|=(1<<nbr);
				pthread_mutex_unlock(&mutexEnd);
			}
			u->listed = false;

			reset(u->set[OUT]);

			for (i = 0; i < u->nsucc; ++i) {
				us = u->succ[i];
				pthread_mutex_lock(&(us->mutexIn));
				or (u->set[OUT], u->set[OUT], us->set[IN]);
				pthread_mutex_unlock(&(us->mutexIn));
			}

			prev = u->prev;
			u->prev = u->set[IN];
			u->set[IN] = prev;
			bool eq;
			pthread_mutex_lock(&(u->mutexIn));
			propagate(u->set[IN], u->set[OUT], u->set[DEF], u->set[USE]);
			eq = equal(u->prev, u->set[IN]);
			pthread_mutex_unlock(&(u->mutexIn));

			if (u->pred != NULL && !eq) {
				p = h = u->pred;
				do {
					v = p->data;
					size_t addr = ((size_t)v & (3 << 7)) >> 7;
					
					if (!v->listed) {
						v->listed = true;
						pthread_mutex_lock(&mutexWork[addr]);
						insert_last(&worklist[addr], v);
						pthread_mutex_unlock(&mutexWork[addr]);
					}
					
					p = p->succ;

				} while (p != h);
			}
			pthread_mutex_lock(&mutexWork[nbr]);
			u = remove_first(&worklist[nbr]);
			pthread_mutex_unlock(&mutexWork[nbr]);
		}
		pthread_mutex_lock(&mutexEnd);
		done = end;
		pthread_mutex_unlock(&mutexEnd);
	}
	return NULL;
}

void liveness(cfg_t* cfg){
	vertex_t*	u;
	size_t		i;
	pthread_t	thread[NT];

	pthread_mutex_init(&mutexEnd, NULL);

	for (i = 0; i < NT; ++i) {
		worklist[i] = NULL;
		pthread_mutex_init(&mutexWork[i], NULL);
	}

	for (i = 0; i < cfg->nvertex; ++i) {
		u = &cfg->vertex[i];
		size_t addr = ((size_t)u&(3<<7))>>7;
		pthread_mutex_init(&(u->mutexIn), NULL);
		insert_last(&worklist[addr], u);
		u->listed = true;
	}

	for (i = 0; i < NT; ++i) {
		pthread_create(&thread[i], NULL, lthread, (void*)i);
	}
	for (i = 0; i < NT; ++i) {
		pthread_join(thread[i], NULL);
	}

	
}

void print_sets(cfg_t* cfg, FILE *fp){
	size_t		i;
	vertex_t*	u;

	for (i = 0; i < cfg->nvertex; ++i) {
		u = &cfg->vertex[i]; 
		fprintf(fp, "use[%zu] = ", u->index);
		print_set(u->set[USE], fp);
		fprintf(fp, "def[%zu] = ", u->index);
		print_set(u->set[DEF], fp);
		fputc('\n', fp);
		fprintf(fp, "in[%zu] = ", u->index);
		print_set(u->set[IN], fp);
		fprintf(fp, "out[%zu] = ", u->index);
		print_set(u->set[OUT], fp);
		fputc('\n', fp);
	}
}
