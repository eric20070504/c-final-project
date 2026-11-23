#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"

int main(void) {
    init_system(); /* 呼叫我們先前寫好的初始化 */

    char cmd[128];
    /* 讀取字串直到遇到 EOF 或跳出 */
    while (scanf("%s", cmd) == 1) {
        if (strcmp(cmd, "QUIT") == 0) {
            /* 這裡之後會補上釋放記憶體的邏輯 */
            printf("BYE\n");
            break;
        }
        else if (strcmp(cmd, "IMPORT") ==0) {
            char filename[128];
            scanf("%s", filename); /* 檔名不會有空白，直接用 scanf 讀取[cite: 2] */
            handle_import(filename);
        }
         else if (strcmp(cmd, "QUERY")==0) {
            char pid[16];
            scanf("%s", pid);
            handle_query(pid);
        }
        else if (strcmp(cmd, "PRICE") == 0) {
            char pid[16];
            int price;
            scanf("%s %d", pid, &price); /* 讀取 ID 與價格數字[cite: 2] */
            handle_price(pid, price);
        }
        else if (strcmp(cmd, "PROMO") ==0){
            char A[16], B[16];
            int N, P;
            scanf("%s %d %s %d", A, &N, B, &P);
            handle_promo(A, N, B, P);
        }
        else if (strcmp(cmd, "CART") == 0) {
            char cid[16];
            scanf("%s", cid);
            handle_cart(cid);
        } else if (strcmp(cmd, "ADD") == 0) {
            char cid[16], pid[16];
            int qty;
            scanf("%s %s %d", cid, pid, &qty);
            handle_add(cid, pid, qty);
        }
        else if (strcmp(cmd, "REPORT") ==0){
            handle_report();
        }
        else if (strcmp(cmd, "CHECKOUT") == 0) {
            char cid[16];
            scanf("%s", cid);
            handle_checkout(cid);
        }
        if (strcmp(cmd, "QUIT") == 0) {
            free_all_data(); /* 結束前釋放所有動態記憶體 */
            printf("BYE\n");
            break;
        }
        // ... (中間原本的指令維持不變) ...
        else if (strcmp(cmd, "SAVE") == 0) {
            char filename[128];
            scanf("%s", filename);
            handle_save(filename);
        }
        else if (strcmp(cmd, "LOAD") == 0) {
            char filename[128];
            scanf("%s", filename);
            handle_load(filename);
        }
    }
    return 0;
}
