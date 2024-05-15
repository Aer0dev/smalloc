#include <stdio.h>
#include <stdint.h>
#include "smalloc.h"

int main() {
    void *p1, *p2, *p3, *p4, *p5, *p6;
    void *bf1, *bf2, *bf3, *bf4;
    void *wf1, *wf2, *wf3, *wf4;

    printf("Initial memory state:\n");
    smdump();

    p1 = smalloc(1000);
    printf("smalloc(1000): %p\n", p1);
    smdump();

    p2 = smalloc(2000);
    printf("smalloc(2000): %p\n", p2);
    smdump();

    p3 = smalloc(3000);
    printf("smalloc(3000): %p\n", p3);
    smdump();

    sfree(p2);
    printf("sfree(%p)\n", p2);
    smdump();

    sfree(p1);
    printf("sfree(%p)\n", p1);
    smdump();

    smcoalesce();
    printf("smcoalesce()\n");
    smdump();

    // Test smalloc_mode with firstfit
    p4 = smalloc_mode(1500, firstfit);
    printf("smalloc_mode(1500, firstfit): %p\n", p4);
    smdump();

    // Test smalloc_mode with bestfit
    bf1 = smalloc_mode(500, bestfit);
    printf("smalloc_mode(500, bestfit): %p\n", bf1);
    smdump();

    bf2 = smalloc_mode(2500, bestfit);
    printf("smalloc_mode(2500, bestfit): %p\n", bf2);
    smdump();

    // Test smalloc_mode with worstfit
    wf1 = smalloc_mode(500, worstfit);
    printf("smalloc_mode(500, worstfit): %p\n", wf1);
    smdump();

    wf2 = smalloc_mode(1000, worstfit);
    printf("smalloc_mode(1000, worstfit): %p\n", wf2);
    smdump();

    // Free some memory and test again
    sfree(p3);
    printf("sfree(%p)\n", p3);
    smdump();

    smcoalesce();
    printf("smcoalesce()\n");
    smdump();

    bf3 = smalloc_mode(1000, bestfit);
    printf("smalloc_mode(1000, bestfit): %p\n", bf3);
    smdump();

    wf3 = smalloc_mode(2000, worstfit);
    printf("smalloc_mode(2000, worstfit): %p\n", wf3);
    smdump();

    return 0;
}