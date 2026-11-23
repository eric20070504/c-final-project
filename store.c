#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "store.h"

/* 內部輔助函式前置宣告 */
Product* find_or_create_product(const char *pid, const char *name);
void add_batch_to_product(Product *prod, int qty, int cost);

/* 在檔案上方、原本的全域變數底下實體化 */
int has_promo = 0;
char promo_A[16] = "";
int promo_N = 0;
char promo_B[16] = "";
int promo_P = 100;
/* 在檔案上方實體化全域變數 */
Cart open_carts[10];
/* 實體化財務變數 */
int total_revenue = 0;
int total_cogs = 0;
int total_gift_cost = 0;

/* 實體化全域變數 */
Product *product_list_head = NULL;
int next_batch_id = 1; /* 剛開始系統預設為 1[cite: 2] */

void init_system() {
    product_list_head = NULL;
    next_batch_id = 1;
    has_promo = 0;
    for (int i = 0; i < 10; i++) {
        open_carts[i].in_use = 0;
        open_carts[i].item_head = NULL;
    }
    total_revenue = 0;
    total_cogs = 0;
    total_gift_cost = 0;
}
/* 請確保 store.c 上方有 #include <string.h> */

/* 處理 CART 指令：建立空購物車 */
void handle_cart(const char *cid) {
    /* 1. 先檢查是否已有同名的開啟中購物車 */
    for (int i = 0; i < 10; i++) {
        if (open_carts[i].in_use == 1 && strcmp(open_carts[i].cid, cid) == 0) {
            printf("ERR DUP_CART\n");
            return;
        }
    }

    /* 2. 再尋找空位建立購物車 */
    for (int i = 0; i < 10; i++) {
        if (open_carts[i].in_use == 0) {
            open_carts[i].in_use = 1;
            strcpy(open_carts[i].cid, cid);
            open_carts[i].item_head = NULL;
            printf("OK CART %s\n", cid);
            return;
        }
    }

    /* 3. 10 個位置全滿 */
    printf("ERR TOO_MANY_CARTS\n");
}
void handle_add(const char *cid, const char *pid, int qty) {
    /* 1. 先檢查購物車是否存在 */
    Cart *target_cart = NULL;
    for (int i = 0; i < 10; i++) {
        if (open_carts[i].in_use == 1 && strcmp(open_carts[i].cid, cid) == 0) {
            target_cart = &open_carts[i];
            break;
        }
    }
    if (target_cart == NULL) {
        printf("ERR NO_CART\n");
        return;
    }

    /* 2. 再檢查商品是否存在 */
    Product *curr_prod = product_list_head;
    int found_prod = 0;
    while (curr_prod != NULL) {
        if (strcmp(curr_prod->id, pid) == 0) {
            found_prod = 1;
            break;
        }
        curr_prod = curr_prod->next;
    }
    if (!found_prod) {
        printf("ERR NO_PRODUCT\n");
        return;
    }

    /* 3. 加入購物車 (若商品已存在則累加，否則新增節點) */
    CartItem *curr_item = target_cart->item_head;
    CartItem *prev_item = NULL;
    while (curr_item != NULL) {
        if (strcmp(curr_item->pid, pid) == 0) {
            curr_item->qty += qty;
            printf("OK ADD %s %s %d\n", cid, pid, curr_item->qty);
            return;
        }
        prev_item = curr_item;
        curr_item = curr_item->next;
    }

    /* 若購物車內還沒有這個商品，則新增一個 CartItem 節點串在尾端 */
    CartItem *new_item = (CartItem *)malloc(sizeof(CartItem));
    strcpy(new_item->pid, pid);
    new_item->qty = qty;
    new_item->next = NULL;

    if (prev_item == NULL) {
        target_cart->item_head = new_item;
    } else {
        prev_item->next = new_item;
    }

    printf("OK ADD %s %s %d\n", cid, pid, qty);
}

/* 內部輔助函式：將批次加入該產品的尾端 (FIFO) */
void add_batch_to_product(Product *prod, int qty, int cost) {
    Batch *new_batch = (Batch *)malloc(sizeof(Batch));
    new_batch->batch_id = next_batch_id++; /* 全域 ID 遞增[cite: 2] */
    new_batch->qty = qty;
    new_batch->unit_cost = cost;
    new_batch->next = NULL;

    if (prod->batch_tail == NULL) {
        prod->batch_head = new_batch; /* 第一個批次 */
    } else {
        prod->batch_tail->next = new_batch; /* 接在最後面 */
    }
    prod->batch_tail = new_batch;
}

/* 內部輔助函式：尋找產品，若無則照 ASCII 順序新增一個 */
Product* find_or_create_product(const char *pid, const char *name) {
    Product *curr = product_list_head;
    Product *prev = NULL;

    /* ID 需以 ASCII 順序比較與排列 */
    while (curr != NULL && strcmp(curr->id, pid) < 0) {
        prev = curr;
        curr = curr->next;
    }

    /* 找到了直接回傳 */
    if (curr != NULL && strcmp(curr->id, pid) == 0) {
        return curr;
    }

    /* 找不到，建立新產品 (插入在 prev 與 curr 之間) */
    Product *new_prod = (Product *)malloc(sizeof(Product));
    strcpy(new_prod->id, pid);
    strcpy(new_prod->name, name);
    new_prod->price = -1;
    new_prod->batch_head = NULL;
    new_prod->batch_tail = NULL;
    new_prod->next = curr;

    if (prev == NULL) {
        product_list_head = new_prod; /* 插入在最前面 */
    } else {
        prev->next = new_prod;        /* 插入在中間或尾端 */
    }
    return new_prod;
}
void handle_import(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("ERR FILE_OPEN\n");
        return;
    }

    char line[256];
    int imported_batches = 0;

    /* 逐行讀取 */
    while (fgets(line, sizeof(line), fp)) {
        /* 移除換行符號 (相容 Windows \r\n 與 Linux \n) */
        line[strcspn(line, "\r\n")] = '\0';
        /* 防呆：略過空行 */
        if (strlen(line) == 0) continue;

        char *hash = strstr(line, "###");
        char *at = strstr(hash ? hash : "", "@@@");
        char *amp = strstr(at ? at : "", "&&&");

        if (hash && at && amp) {
            /* 把分隔符號的起點改成 \0，藉此截斷字串 */
            *hash = '\0';
            *at = '\0';
            *amp = '\0';

            char *pid = line;           /* 開頭到第一個 \0 是 ID */
            char *name = hash + 3;      /* ### 之後是 Name */
            char *qty_str = at + 3;     /* @@@ 之後是 Qty */
            char *cost_str = amp + 3;   /* &&& 之後是 Cost */

            int qty = atoi(qty_str);
            int cost = atoi(cost_str);

            Product *prod = find_or_create_product(pid, name);
            add_batch_to_product(prod, qty, cost);

            imported_batches++;
        }
    }
    fclose(fp);

    /* 成功匯入後印出結果[cite: 3] */
    printf("OK IMPORT %d\n", imported_batches);
}
void handle_query(const char *pid) {
    Product *curr = product_list_head;

    /* 尋找指定的產品 */
    while (curr != NULL) {
        if (strcmp(curr->id, pid) == 0) {
            break;
        }
        curr = curr->next;
    }

    /* 找不到產品的錯誤處理[cite: 4] */
    if (curr == NULL) {
        printf("ERR NO_PRODUCT\n");
        return;
    }

    /* 計算總庫存量 */
    int total_stock = 0;
    Batch *b = curr->batch_head;
    while (b != NULL) {
        total_stock += b->qty;
        b = b->next;
    }

    /* 印出第一行資訊：注意 price=none 的特殊情況[cite: 5] */
    if (curr->price == -1) {
        printf("QUERY %s %s price=none stock=%d\n", curr->id, curr->name, total_stock);
    } else {
        printf("QUERY %s %s price=%d stock=%d\n", curr->id, curr->name, curr->price, total_stock);
    }

    /* 若有庫存，依序印出每個批次，前方需有 2 個空白[cite: 4, 5] */
    if (total_stock > 0) {
        b = curr->batch_head;
        while (b != NULL) {
            printf("  #%d qty=%d cost=%d\n", b->batch_id, b->qty, b->unit_cost);
            b = b->next;
        }
    }
}
void handle_price(const char *pid, int price) {
    Product *curr = product_list_head;

    /* 尋找指定的產品 */
    while (curr != NULL) {
        if (strcmp(curr->id, pid) == 0) {
            curr->price = price; /* 更新價格[cite: 2] */
            printf("OK PRICE %s %d\n", curr->id, curr->price);
            return;
        }
        curr = curr->next;
    }

    /* 找不到該產品的錯誤處理[cite: 4] */
    printf("ERR NO_PRODUCT\n");
}
void handle_promo(const char *A, int N, const char *B, int P) {
    has_promo = 1;
    strcpy(promo_A, A);
    promo_N = N;
    strcpy(promo_B, B);
    promo_P = P;

    /* 成功設定後印出結果[cite: 2] */
    printf("OK PROMO\n");
}
void handle_report(void) {
    printf("REPORT\n");
    Product *curr = product_list_head;

    /* 依序走訪所有產品（因為我們一開始就是按 ID 順序插入的，所以直接走訪即可）[cite: 5] */
    while (curr != NULL) {
        int stock = 0;
        int value = 0;
        Batch *b = curr->batch_head;

        /* 計算該產品所有剩餘批次的庫存與總值[cite: 5] */
        while (b != NULL) {
            stock += b->qty;
            value += (b->qty * b->unit_cost);
            b = b->next;
        }

        /* 只印出庫存大於 0 的產品[cite: 5] */
        if (stock > 0) {
            printf("  %s stock=%d value=%d\n", curr->id, stock, value);
        }
        curr = curr->next;
    }
    /* 計算並印出累積財務數據[cite: 5] */
    int profit = total_revenue - total_cogs - total_gift_cost;
    printf("REVENUE %d\n", total_revenue);
    printf("COGS %d\n", total_cogs);
    printf("GIFTCOST %d\n", total_gift_cost);
    printf("PROFIT %d\n", profit);
}
/* 輔助函式 1：關閉購物車並釋放所有 CartItem 記憶體 */
void close_cart(Cart *cart) {
    CartItem *curr = cart->item_head;
    while (curr != NULL) {
        CartItem *next = curr->next;
        free(curr);
        curr = next;
    }
    cart->item_head = NULL;
    cart->in_use = 0;
}

/* 輔助函式 2：計算某產品目前的總庫存 */
int get_product_stock(const Product *p) {
    int total = 0;
    for (const Batch *b = p->batch_head; b != NULL; b = b->next) {
        total += b->qty;
    }
    return total;
}

/* 輔助函式 3：依 FIFO 扣除庫存、立刻釋放用完的批次，並回傳總成本 */
int consume_stock_fifo(Product *p, int qty_needed) {
    int cost = 0;
    while (qty_needed > 0 && p->batch_head != NULL) {
        Batch *b = p->batch_head;
        int take = (b->qty < qty_needed) ? b->qty : qty_needed;

        cost += take * b->unit_cost;
        b->qty -= take;
        qty_needed -= take;

        /* 批次用完立刻移除並 free (參考 Appendix B，注意 tail 歸 NULL) */
        if (b->qty == 0) {
            p->batch_head = b->next;
            if (p->batch_head == NULL) {
                p->batch_tail = NULL;
            }
            free(b);
        }
    }
    return cost;
}
/* 輔助函式 4：在購物車中尋找特定商品 */
CartItem* find_cart_item(Cart *cart, const char *pid) {
    for (CartItem *it = cart->item_head; it != NULL; it = it->next) {
        if (strcmp(it->pid, pid) == 0) return it;
    }
    return NULL;
}

void handle_checkout(const char *cid) {
    /* 尋找購物車 */
    Cart *cart = NULL;
    for (int i = 0; i < 10; i++) {
        if (open_carts[i].in_use == 1 && strcmp(open_carts[i].cid, cid) == 0) {
            cart = &open_carts[i];
            break;
        }
    }
    if (cart == NULL) {
        printf("ERR NO_CART\n");
        return;
    }

    /* 步驟 1：依產品 ID 順序檢查價格與庫存 */
    for (Product *p = product_list_head; p != NULL; p = p->next) {
        CartItem *it = find_cart_item(cart, p->id);
        if (it == NULL) continue;

        if (p->price == -1) {
            printf("ERR NO_PRICE\n");
            close_cart(cart); /* 失敗也要關閉購物車 */
            return;
        }
        if (get_product_stock(p) < it->qty) {
            printf("ERR INSUFFICIENT %s\n", p->id);
            close_cart(cart);
            return;
        }
    }

    /* 判斷促銷條件是否達成 (A 產品數量 >= N) */
    int promo_active = 0;
    if (has_promo) {
        CartItem *item_A = find_cart_item(cart, promo_A);
        if (item_A != NULL && item_A->qty >= promo_N) {
            promo_active = 1;
        }
    }

    /* 步驟 2 & 3：印出收據、計算金額並依 FIFO 扣除庫存 */
    printf("RECEIPT %s\n", cid);
    int total = 0;
    int cost = 0;

    for (Product *p = product_list_head; p != NULL; p = p->next) {
        CartItem *it = find_cart_item(cart, p->id);
        if (it == NULL) continue;

        int is_discounted = (promo_active && strcmp(p->id, promo_B) == 0);
        int P = is_discounted ? promo_P : 100;
        int subtotal = (int)(((long long)p->price * it->qty * P + 50) / 100);

        if (is_discounted) {
            printf("  %s x%d @%d *%d%% = %d\n", p->id, it->qty, p->price, P, subtotal);
        } else {
            printf("  %s x%d @%d = %d\n", p->id, it->qty, p->price, subtotal);
        }

        total += subtotal;
        cost += consume_stock_fifo(p, it->qty);
    }

    printf("TOTAL %d\n", total);
    printf("COST %d\n", cost);

    /* 步驟 4：計算並發放滿千贈品 */
    int target_gifts = total / 1000;
    int gifts_given = 0;
    int gift_cost = 0;

    for (int g = 0; g < target_gifts; g++) {
        Product *best_gift = NULL;
        int min_diff = 0;

        for (Product *p = product_list_head; p != NULL; p = p->next) {
            /* 必須有定價且有庫存 */
            if (p->price == -1 || p->batch_head == NULL) continue;

            int diff = p->price - p->batch_head->unit_cost;
            /* 嚴格小於 (<) 會在平手時自動保留 ID 較小的產品 */
            if (best_gift == NULL || diff < min_diff) {
                best_gift = p;
                min_diff = diff;
            }
        }

        if (best_gift == NULL) break; /* 沒東西可送就提早結束 */

        int unit_cost = consume_stock_fifo(best_gift, 1);
        gift_cost += unit_cost;
        gifts_given++;
        printf("GIFT %s @%d\n", best_gift->id, unit_cost);
    }

    if (gifts_given == 0) {
        printf("GIFT none\n");
    }
    if (gifts_given < target_gifts) {
        printf("GIFT_SHORT %d\n", target_gifts - gifts_given);
    }

    /* 步驟 5：更新全域財務統計、印出利潤並關閉購物車 */
    int profit = total - cost - gift_cost;
    total_revenue += total;
    total_cogs += cost;
    total_gift_cost += gift_cost;

    printf("PROFIT %d\n", profit);
    close_cart(cart);
}
void free_all_data(void) {
    /* 1. 關閉並釋放所有購物車 */
    for (int i = 0; i < 10; i++) {
        if (open_carts[i].in_use) {
            close_cart(&open_carts[i]);
        }
    }

    /* 2. 釋放所有產品與其底下的批次 (先釋放內層 Batch，再釋放外層 Product) */
    Product *p = product_list_head;
    while (p != NULL) {
        Product *next_p = p->next;
        Batch *b = p->batch_head;
        while (b != NULL) {
            Batch *next_b = b->next;
            free(b);
            b = next_b;
        }
        free(p);
        p = next_p;
    }

    /* 3. 重置所有全域變數 */
    init_system();
}
void handle_save(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("ERR FILE_OPEN\n");
        return;
    }

    /* 計算產品總數與剩餘批次總數 */
    int product_count = 0;
    int batch_count = 0;
    for (Product *p = product_list_head; p != NULL; p = p->next) {
        product_count++;
        for (Batch *b = p->batch_head; b != NULL; b = b->next) {
            if (b->qty > 0) batch_count++;
        }
    }

    /* 1. NEXT_BATCH */
    fprintf(fp, "NEXT_BATCH %d\n", next_batch_id);

    /* 2. PRODUCTS (已按 ID 排序) */
    fprintf(fp, "PRODUCTS %d\n", product_count);
    for (Product *p = product_list_head; p != NULL; p = p->next) {
        fprintf(fp, "%s %d\n", p->id, p->price);
        fprintf(fp, "%s\n", p->name);
    }

    /* 3. PROMO */
    fprintf(fp, "PROMO %d\n", has_promo);
    if (has_promo) {
        fprintf(fp, "%s %d %s %d\n", promo_A, promo_N, promo_B, promo_P);
    }

    /* 4. TOTALS */
    fprintf(fp, "TOTALS %d %d %d\n", total_revenue, total_cogs, total_gift_cost);

    /* 5. BATCHES (已按 ID 與 FIFO 排序) */
    fprintf(fp, "BATCHES %d\n", batch_count);
    for (Product *p = product_list_head; p != NULL; p = p->next) {
        for (Batch *b = p->batch_head; b != NULL; b = b->next) {
            if (b->qty > 0) {
                fprintf(fp, "%s %d %d %d\n", p->id, b->batch_id, b->qty, b->unit_cost);
            }
        }
    }

    fclose(fp);
    printf("OK SAVE\n");
}
/* 內部輔助函式：載入指定 batch_id 的批次 (不遞增 next_batch_id) */
void load_batch_to_product(Product *prod, int batch_id, int qty, int cost) {
    Batch *new_batch = (Batch *)malloc(sizeof(Batch));
    new_batch->batch_id = batch_id;
    new_batch->qty = qty;
    new_batch->unit_cost = cost;
    new_batch->next = NULL;

    if (prod->batch_tail == NULL) {
        prod->batch_head = new_batch;
    } else {
        prod->batch_tail->next = new_batch;
    }
    prod->batch_tail = new_batch;
}

void handle_load(const char *filename) {
    /* 先確認檔案打得開，打不開就不動原本的資料 */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("ERR FILE_OPEN\n");
        return;
    }

    /* 確定開檔成功，清空目前所有資料並關閉購物車 */
    free_all_data();

    char line[256];
    int count = 0;

    /* 1. 讀取 NEXT_BATCH */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "NEXT_BATCH %d", &next_batch_id);
    }

    /* 2. 讀取 PRODUCTS */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "PRODUCTS %d", &count);
        for (int i = 0; i < count; i++) {
            char pid[16];
            int price;
            fgets(line, sizeof(line), fp);
            sscanf(line, "%s %d", pid, &price);

            /* 讀取完整產品名稱並去掉結尾換行 */
            fgets(line, sizeof(line), fp);
            line[strcspn(line, "\r\n")] = '\0';

            Product *p = find_or_create_product(pid, line);
            p->price = price;
        }
    }

    /* 3. 讀取 PROMO */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "PROMO %d", &has_promo);
        if (has_promo) {
            fgets(line, sizeof(line), fp);
            sscanf(line, "%s %d %s %d", promo_A, &promo_N, promo_B, &promo_P);
        }
    }

    /* 4. 讀取 TOTALS */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "TOTALS %d %d %d", &total_revenue, &total_cogs, &total_gift_cost);
    }

    /* 5. 讀取 BATCHES */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "BATCHES %d", &count);
        for (int i = 0; i < count; i++) {
            char pid[16];
            int bid, qty, cost;
            fgets(line, sizeof(line), fp);
            sscanf(line, "%s %d %d %d", pid, &bid, &qty, &cost);

            /* 找到對應產品並掛上批次 */
            for (Product *p = product_list_head; p != NULL; p = p->next) {
                if (strcmp(p->id, pid) == 0) {
                    load_batch_to_product(p, bid, qty, cost);
                    break;
                }
            }
        }
    }

    fclose(fp);
    printf("OK LOAD\n");
}

