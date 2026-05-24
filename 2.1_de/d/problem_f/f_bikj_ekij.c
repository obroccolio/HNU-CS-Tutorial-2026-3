#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#define M 1001
#define K 1001
#define N 1001
int main(){
    double *A=malloc(M*K*8),*B=malloc(K*N*8),*C_no=calloc(M*N,8),*C_yes=calloc(M*N,8),*B_T=malloc(K*N*8);
    memset(A,0,M*K*8);memset(B,0,K*N*8);
    for(int i=0;i<M*K;i++)A[i]=1.0+(i%1000)*0.001;
    for(int i=0;i<K*N;i++)B[i]=1.0+(i%1000)*0.001;
    struct timespec s,e;
    clock_gettime(CLOCK_MONOTONIC,&s);
    {double *C=C_no;for(int ii=0;ii<M;ii+=64)for(int kk=0;kk<K;kk+=64)for(int jj=0;jj<N;jj+=64)for(int i=ii;i<ii+64&&i<M;i++)for(int k=kk;k<kk+64&&k<K;k++)for(int j=jj;j<jj+64&&j<N;j++)C[i*N+j]+=A[i*K+k]*B[k*N+j];;}
    clock_gettime(CLOCK_MONOTONIC,&e);
    double t_no=(e.tv_sec-s.tv_sec)*1000+(e.tv_nsec-s.tv_nsec)/1e6;
    clock_gettime(CLOCK_MONOTONIC,&s);
    for(int k=0;k<K;k++)for(int j=0;j<N;j++)B_T[j*K+k]=B[k*N+j];
    clock_gettime(CLOCK_MONOTONIC,&e);
    double t_trans=(e.tv_sec-s.tv_sec)*1000+(e.tv_nsec-s.tv_nsec)/1e6;
    clock_gettime(CLOCK_MONOTONIC,&s);
    {double *C=C_yes;for(int ii=0;ii<M;ii+=64)for(int kk=0;kk<K;kk+=64)for(int jj=0;jj<N;jj+=64)for(int i=ii;i<ii+64&&i<M;i++)for(int k=kk;k<kk+64&&k<K;k++)for(int j=jj;j<jj+64&&j<N;j++)C[i*N+j]+=A[i*K+k]*B_T[j*K+k];;}
    clock_gettime(CLOCK_MONOTONIC,&e);
    double t_mul=(e.tv_sec-s.tv_sec)*1000+(e.tv_nsec-s.tv_nsec)/1e6;
    double t_total=t_trans+t_mul,speedup=t_no/t_total,improv=100*(1-1/speedup);
    printf("block_ikj<elem_kij|%.2f|%.2f|%.2f|%.2f|%.2fx|%+.2f%%\n",t_no,t_total,t_trans,t_mul,speedup,improv);
    free(A);free(B);free(C_no);free(C_yes);free(B_T);
    return 0;}