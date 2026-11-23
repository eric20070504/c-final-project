#ifndef STORE_H
#define STORE_H
/* 批次 (Batch) 節點：必須符合 FIFO (先進先出) */
typedef struct Batch {
    int batch_id;        /* 全域批次 ID */
    int qty;             /* 該批次的剩餘數量 */
    int unit_cost;       /* 該批次的單位成本 */
    struct Batch *next;  /* 指向下一個較新的批次 */
} Batch;

/* 產品 (Product) 節點 */
typedef struct Product {
    char id[16];         /* 產品 ID (15字元 + '\0')[cite: 1] */
    char name[32];       /* 產品名稱 (31字元 + '\0')[cite: 1] */
    int price;           /* 產品售價 (-1 代表尚未定價) */

    Batch *batch_head;   /* 指向該產品的最舊批次 (FIFO 扣除用) */
    Batch *batch_tail;   /* 指向該產品的最新批次 (方便從尾巴新增) */

    struct Product *next;/* 指向下一個產品 */
} Product;

/* 購物車內的商品節點 (使用 Linked List 以便無限制新增) */
typedef struct CartItem {
    char pid[16];
    int qty;                 /* 在購物車內的總數量 */
    struct CartItem *next;   /* 指向下一個購物車商品 */
} CartItem;

/* 購物車本體 (我們將用陣列來儲存最多 10 個) */
typedef struct Cart {
    int in_use;              /* 0 代表空閒，1 代表使用中 */
    char cid[16];            /* 購物車 ID */
    CartItem *item_head;     /* 購物車內的商品串列開頭 */
} Cart;

/* 在 extern 全域變數區加入 */
extern Cart open_carts[10];  /* 最多 10 個開啟的購物車[cite: 2] */

/* 系統全域變數宣告 (使用 extern 讓 main.c 也能看見) */
extern Product *product_list_head; /* 產品串列的開頭 */
extern int next_batch_id;          /* 下一個要分配的全域批次 ID */
/* 促銷規則全域變數宣告 (extern 讓其他檔案也能讀取) */
extern int has_promo;      /* 0 代表無促銷，1 代表有促銷 */
extern char promo_A[16];   /* 條件產品 A[cite: 2] */
extern int promo_N;        /* 條件數量 N[cite: 2] */
extern char promo_B[16];   /* 折扣產品 B[cite: 2] */
extern int promo_P;        /* 折扣百分比 P[cite: 2] */
/* 財務統計全域變數 */
extern int total_revenue;
extern int total_cogs;
extern int total_gift_cost;

/* 函式宣告 */
void init_system();                /* 初始化系統狀態 */

// 未來我們還會在這裡加入 import_stock, set_price 等函式

/* 在 store.h 的 #endif 之前加入 */
void handle_import(const char *filename);
void handle_query(const char *pid);
void handle_price(const char *pid, int price);
void handle_promo(const char *A, int N, const char *B, int P);
void handle_cart(const char *cid);
void handle_add(const char *cid, const char *pid, int qty);
void handle_report(void);
void handle_checkout(const char *cid);
void free_all_data(void);
void handle_save(const char *filename);
void handle_load(const char *filename);

#endif
