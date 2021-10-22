#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <omp.h>

#define ERROR -20

struct Stack {
    int * arr;
    int max_size;
    int top;
};

struct Graph {
    int E;
    int V;
    int ** adj; // adjacency matrix
};

void init_graph_auto(struct Graph *G, int V, int E);
void init_graph(struct Graph *G);
void check_hamiltonian(struct Graph G);
int num_hamiltonian_cycles(int position, int * visit, struct Graph G, struct Stack *Path);
void push(struct Stack *S, int data);
int pop(struct Stack *S);
void show_stack(struct Stack *S);
void print_graph(struct Graph *G);
void copy_path(struct Stack * st, struct Stack * copy_st);
void copy_visit(int * visit, int * copy_visit, int len);

int main() {
    struct Graph G;
    // init_graph_auto(&G, 40, 165);
    init_graph(&G);
    int num_threads [] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 32, 64, 80, 100, 128, 200, 256, 512};
    int num_threads_size = 19;
    // int num_threads [] = {1, 2, 4, 6, 8, 10};
    // int num_threads_size = 6;
    double start_time, end_time;
    for(int j = 0; j < num_threads_size; j++) {    // for various number of threads
        start_time = omp_get_wtime();   // record the starting time
        omp_set_num_threads(num_threads[j]);   // set the number of threads required
        check_hamiltonian(G);
        end_time = omp_get_wtime();    // record the ending time
        printf("#threads: %d     Running time: %f\n", num_threads[j], end_time - start_time);
    }
    
    for (int i = 0; i < G.V; i++) 
        free(G.adj[i]);
    free(G.adj);
    return 0;
}


/********************** stack operations implementation *******************/
void push(struct Stack *S, int data) {
    if(S->top > S->max_size - 1)
        printf("Stack overflow\n");
    else {
        S->top++;
        S->arr[S->top] = data;
    }
}

int top(struct Stack *S) {
    if(S->top > -1) 
        return S->arr[S->top];
    printf("Stack is empty!\n");
    return ERROR;

}

int pop(struct Stack *S) {
    if(S->top > -1) {
        int val = S->arr[S->top];
        S->top--;
        return val;
    }
    printf("Popping out of empty stack\n");
    return ERROR;
}

void show_stack(struct Stack *S) {
    printf("[");
    for(int i = 0; i <= S->top; i++) {
        printf("%d ", S->arr[i]);
    }
    printf("\b<---top\n");
}


/******************** Graph Initialization related ************************/

// initializes an undirected graph with V nodes and E edges
void init_graph_auto(struct Graph *G, int V, int E) {
    G->V = V;
    G->E = E;
    srand(time(0));
    G->adj = (int**)malloc(sizeof(int*)*G->V);
    for(int i = 0; i < G->V; i++) {
        G->adj[i] = (int *)malloc(sizeof(int)*G->V);
        memset(G->adj[i], 0, sizeof(int)*G->V);
    }
    int u, v;
    for(int i = 0; i < G->E; i++) {
        u = rand() % G->V;
        v = rand() % G->V;
        G->adj[u][v] = 1;
        G->adj[v][u] = 1;
    }
}

// initializes an undirected graph based on the user input
void init_graph(struct Graph *G) {
    scanf("%d %d", &G->V, &G->E);
    G->adj = (int**)malloc(sizeof(int*)*G->V);
    for(int i = 0; i < G->V; i++) {
        G->adj[i] = (int *)malloc(sizeof(int)*G->V);
        memset(G->adj[i], 0, sizeof(int)*G->V);
    }
    int u, v;
    for(int i = 0; i < G->E; i++) {
        scanf("%d %d", &u, &v);
        G->adj[v][u] = 1;
        G->adj[u][v] = 1;
    }
}

// prints the graph
void print_graph(struct Graph *G) {
    for(int i = 0; i < G->V; i++) {
        printf("%d: ", i);
        for(int j = 0; j < G->V; j++) {
            printf("%d ", G->adj[i][j]);
        }
        printf("\n");
    }   
}

/************************ Counting Hamiltonian cycles *********************/

void check_hamiltonian(struct Graph G) {
    int visit[G.V];
    for(int i = 0; i < G.V; i++)
        visit[i] = 0;
    struct Stack Path;
    Path.max_size = G.V + 1;
    Path.top = -1;
    Path.arr = (int*)malloc(sizeof(int)*(G.V + 1));
    push(&Path, 0);
    visit[0] = 1;
    int num_cycles = num_hamiltonian_cycles(1, visit, G, &Path);
    printf("#Hamiltonian Cycles = %d\n", num_cycles); 
}

// returns the number of hamiltonian cycles present in the undirected graph `G`
int num_hamiltonian_cycles(int position, int * visit, struct Graph G, struct Stack *Path) {
    if(position == G.V) {
        if(G.adj[top(Path)][0]) {
            push(Path, 0);
            // show_stack(Path);
            pop(Path);
            return 1;
        }
        return 0;
    }
    int num_cycles = 0;
    #pragma omp parallel shared(visit, num_cycles)
    {
        #pragma omp for reduction(+:num_cycles)
        for(int i = 0; i < G.V; i++) {
            if((G.adj[top(Path)][i] && !visit[i])) {
                int visit_copy[G.V];
                struct Stack Path_copy;
                copy_visit(visit, visit_copy, G.V);
                copy_path(Path, &Path_copy);
                push(&Path_copy, i);
                visit_copy[i] = 1;
                num_cycles += num_hamiltonian_cycles(position + 1, visit_copy, G, &Path_copy);
                visit_copy[i] = 0;
                pop(&Path_copy);
            }
        }
    }
    return num_cycles;
}

// copy Stack `st` to `copy_st`
void copy_path(struct Stack * st, struct Stack * copy_st) {
    copy_st->max_size = st->max_size;
    copy_st->top = st->top;
    copy_st->arr = (int*)malloc(sizeof(int)*(st->max_size));
    for(int i = 0; i < st->max_size; i++)
        copy_st->arr[i] = st->arr[i];
}

// copy array `visit` to `copy_visit`
void copy_visit(int * visit, int * copy_visit, int len) {
    for(int i = 0; i < len; i++)
        copy_visit[i] = visit[i];
}

/* Input types
8 9
0 1
1 2
2 3
3 4
4 1
4 6
3 5
5 6
6 7

ham

8 9
0 1
1 2
2 3
3 4
0 4
4 7
3 5
5 6
6 7



print the graph



17 30
0 1
1 2
2 8
0 8
2 4
2 3
3 4
8 3
3 12
4 12
4 15
14 15
1 9
0 10
9 10
9 14
13 14
13 10
13 16
11 16
10 11
6 11
0 6
5 12
5 6
6 11
7 6
7 5
5 8
3 5
==> 28 cycles

6 8
0 1
1 2
2 5
4 5
3 4
0 3
1 4
2 3
==> 4 cycles

6 8
0 1
1 2
2 5
5 4
4 3
0 3
2 3
1 4

==> 4 cycles


*/