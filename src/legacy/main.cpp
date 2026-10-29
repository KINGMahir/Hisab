#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <climits>
#include <sqlite3.h>

using namespace std;

// --------------------------- Helpers ---------------------------
string nowISO() {
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    return string(buf);
}

string sanitize(const string& s) {
    string r = s;
    // trim
    r.erase(r.begin(), find_if(r.begin(), r.end(), [](unsigned char ch){return !isspace(ch);} ));
    r.erase(find_if(r.rbegin(), r.rend(), [](unsigned char ch){return !isspace(ch);} ).base(), r.end());
    return r;
}

bool toInt(const string& s, int& out) {
    try { size_t idx; int v = stoi(s, &idx); if(idx!=s.size()) return false; out = v; return true; }
    catch(...) { return false; }
}
bool toDouble(const string& s, double& out) {
    try { size_t idx; double v = stod(s, &idx); if(idx!=s.size()) return false; out = v; return true; }
    catch(...) { return false; }
}

int askInt(const string& prompt, int minv=INT_MIN, int maxv=INT_MAX) {
    while(true){
        cout<<prompt;
        string s; getline(cin,s); s=sanitize(s);
        int v; if(toInt(s,v) && v>=minv && v<=maxv) return v;
        cout<<"Invalid integer. Try again.\n";
    }
}
double askDouble(const string& prompt, double minv=-1e18, double maxv=1e18) {
    while(true){
        cout<<prompt;
        string s; getline(cin,s); s=sanitize(s);
        double v; if(toDouble(s,v) && v>=minv && v<=maxv) return v;
        cout<<"Invalid number. Try again.\n";
    }
}
string askText(const string& prompt, bool allowEmpty=false) {
    while(true){
        cout<<prompt;
        string s; getline(cin,s);
        s=sanitize(s);
        if(allowEmpty || !s.empty()) return s;
        cout<<"Cannot be empty. Try again.\n";
    }
}

// --------------------------- DB Wrapper ---------------------------
struct DB {
    sqlite3* db = nullptr;

    bool open(const char* path) {
        if(sqlite3_open(path, &db) != SQLITE_OK) {
            cerr<<"[DB] Open failed: "<<sqlite3_errmsg(db)<<'\n';
            return false;
        }
        return true;
    }
    ~DB(){ if(db) sqlite3_close(db); }

    bool exec(const string& sql) {
        char* err = nullptr;
        if(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err)!=SQLITE_OK) {
            cerr<<"[DB] SQL error: "<<(err?err:"")<<'\n';
            if(err) sqlite3_free(err);
            return false;
        }
        return true;
    }
};

DB gdb;

// --------------------------- Schema ---------------------------
bool initSchema() {
    const char* sql =
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS products ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sku TEXT UNIQUE,"
        "  name TEXT NOT NULL,"
        "  category TEXT,"
        "  price REAL NOT NULL CHECK(price>=0),"
        "  stock INTEGER NOT NULL CHECK(stock>=0)"
        ");"
        "CREATE TABLE IF NOT EXISTS sales ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ts TEXT NOT NULL,"
        "  subtotal REAL NOT NULL,"
        "  vat REAL NOT NULL,"
        "  discount REAL NOT NULL,"
        "  grand_total REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS sale_items ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sale_id INTEGER NOT NULL,"
        "  product_id INTEGER NOT NULL,"
        "  qty INTEGER NOT NULL CHECK(qty>0),"
        "  unit_price REAL NOT NULL,"
        "  line_total REAL NOT NULL,"
        "  FOREIGN KEY(sale_id) REFERENCES sales(id) ON DELETE CASCADE,"
        "  FOREIGN KEY(product_id) REFERENCES products(id)"
        ");";
    return gdb.exec(sql);
}

// --------------------------- Product CRUD ---------------------------
void addProduct() {
    string sku = askText("SKU (unique, can be blank): ", true);
    string name = askText("Product name: ");
    string category = askText("Category (optional): ", true);
    double price = askDouble("Unit price: ", 0);
    int stock = askInt("Initial stock qty: ", 0);

    sqlite3_stmt* st=nullptr;
    const char* sql = "INSERT INTO products (sku,name,category,price,stock) VALUES (?,?,?,?,?);";
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Prepare failed\n"; return; }

    sqlite3_bind_text(st,1, sku.empty()?nullptr:sku.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st,2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st,3, category.empty()?nullptr:category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st,4, price);
    sqlite3_bind_int(st,5, stock);

    if(sqlite3_step(st)!=SQLITE_DONE) cerr<<"Insert failed: "<<sqlite3_errmsg(gdb.db)<<'\n';
    else cout<<"✅ Product added.\n";
    sqlite3_finalize(st);
}

void listProducts() {
    cout<<left<<setw(6)<<"ID"<<setw(12)<<"SKU"<<setw(22)<<"Name"
        <<setw(14)<<"Category"<<right<<setw(10)<<"Price"<<setw(10)<<"Stock"<<'\n';
    cout<<string(78,'-')<<'\n';

    const char* sql = "SELECT id,IFNULL(sku,''),name,IFNULL(category,''),price,stock FROM products ORDER BY id;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Query failed\n"; return; }
    while(sqlite3_step(st)==SQLITE_ROW){
        int id = sqlite3_column_int(st,0);
        string sku = (const char*)sqlite3_column_text(st,1);
        string name = (const char*)sqlite3_column_text(st,2);
        string cat = (const char*)sqlite3_column_text(st,3);
        double price = sqlite3_column_double(st,4);
        int stock = sqlite3_column_int(st,5);
        cout<<left<<setw(6)<<id<<setw(12)<<sku.substr(0,11)<<setw(22)<<name.substr(0,21)
            <<setw(14)<<cat.substr(0,13)<<right<<setw(10)<<fixed<<setprecision(2)<<price<<setw(10)<<stock<<'\n';
    }
    sqlite3_finalize(st);
}

void searchProducts() {
    string q = askText("Search by name/SKU contains: ");
    string like = "%"+q+"%";
    const char* sql = "SELECT id,IFNULL(sku,''),name,IFNULL(category,''),price,stock "
                      "FROM products WHERE name LIKE ? OR sku LIKE ? ORDER BY id;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Prepare failed\n"; return; }
    sqlite3_bind_text(st,1, like.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st,2, like.c_str(), -1, SQLITE_TRANSIENT);

    cout<<left<<setw(6)<<"ID"<<setw(12)<<"SKU"<<setw(22)<<"Name"
        <<setw(14)<<"Category"<<right<<setw(10)<<"Price"<<setw(10)<<"Stock"<<'\n';
    cout<<string(78,'-')<<'\n';
    while(sqlite3_step(st)==SQLITE_ROW){
        int id = sqlite3_column_int(st,0);
        string sku = (const char*)sqlite3_column_text(st,1);
        string name = (const char*)sqlite3_column_text(st,2);
        string cat = (const char*)sqlite3_column_text(st,3);
        double price = sqlite3_column_double(st,4);
        int stock = sqlite3_column_int(st,5);
        cout<<left<<setw(6)<<id<<setw(12)<<sku.substr(0,11)<<setw(22)<<name.substr(0,21)
            <<setw(14)<<cat.substr(0,13)<<right<<setw(10)<<fixed<<setprecision(2)<<price<<setw(10)<<stock<<'\n';
    }
    sqlite3_finalize(st);
}

void updateProduct() {
    int id = askInt("Product ID to update: ", 1);
    cout<<"Leave blank to keep existing.\n";

    string newName = askText("New name: ", true);
    string newCat  = askText("New category: ", true);

    string sPrice = askText("New price: ", true);
    string sStock = askText("New stock: ", true);

    // Build dynamic SQL
    vector<string> sets;
    if(!newName.empty()) sets.push_back("name=?");
    if(!newCat.empty())  sets.push_back("category=?");
    bool chPrice=false, chStock=false;
    double price=0; int stock=0;
    if(!sPrice.empty() && toDouble(sPrice,price) && price>=0){ sets.push_back("price=?"); chPrice=true; }
    if(!sStock.empty() && toInt(sStock,stock) && stock>=0){ sets.push_back("stock=?"); chStock=true; }

    if(sets.empty()){ cout<<"Nothing to update.\n"; return; }

    string sql = "UPDATE products SET ";
    for(size_t i=0;i<sets.size();++i){ if(i) sql+=", "; sql+=sets[i]; }
    sql += " WHERE id=?;";

    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql.c_str(), -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Prepare failed\n"; return; }

    int idx=1;
    if(!newName.empty()) sqlite3_bind_text(st,idx++, newName.c_str(), -1, SQLITE_TRANSIENT);
    if(!newCat.empty())  sqlite3_bind_text(st,idx++, newCat.c_str(), -1, SQLITE_TRANSIENT);
    if(chPrice) sqlite3_bind_double(st,idx++, price);
    if(chStock) sqlite3_bind_int(st,idx++, stock);
    sqlite3_bind_int(st,idx, id);

    if(sqlite3_step(st)!=SQLITE_DONE) cerr<<"Update failed: "<<sqlite3_errmsg(gdb.db)<<'\n';
    else cout<<"✅ Updated.\n";
    sqlite3_finalize(st);
}

void deleteProduct() {
    int id = askInt("Product ID to delete: ", 1);
    sqlite3_stmt* st=nullptr;
    const char* sql="DELETE FROM products WHERE id=?;";
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Prepare failed\n"; return; }
    sqlite3_bind_int(st,1,id);
    if(sqlite3_step(st)!=SQLITE_DONE) cerr<<"Delete failed: "<<sqlite3_errmsg(gdb.db)<<'\n';
    else cout<<"✅ Deleted.\n";
    sqlite3_finalize(st);
}

// --------------------------- Billing ---------------------------
struct CartItem {
    int product_id;
    string name;
    int qty;
    double unit_price;
    double line_total() const { return qty * unit_price; }
};

bool fetchProductById(int id, string& name, double& price, int& stock) {
    const char* sql = "SELECT name, price, stock FROM products WHERE id=?;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK) return false;
    sqlite3_bind_int(st,1,id);
    bool ok=false;
    if(sqlite3_step(st)==SQLITE_ROW){
        name = (const char*)sqlite3_column_text(st,0);
        price = sqlite3_column_double(st,1);
        stock = sqlite3_column_int(st,2);
        ok=true;
    }
    sqlite3_finalize(st);
    return ok;
}

bool updateStockDelta(int id, int delta) {
    const char* sql = "UPDATE products SET stock = stock + ? WHERE id=? AND stock + ? >= 0;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK) return false;
    sqlite3_bind_int(st,1,delta);
    sqlite3_bind_int(st,2,id);
    sqlite3_bind_int(st,3,delta);
    bool ok = (sqlite3_step(st)==SQLITE_DONE && sqlite3_changes(gdb.db)>0);
    sqlite3_finalize(st);
    return ok;
}

void newSale() {
    cout<<"\n=== Start New Sale ===\n";
    vector<CartItem> cart;
    while(true){
        string s = askText("Enter product ID (or 'done' to finish): ");
        if(s=="done" || s=="DONE") break;
        int pid;
        if(!toInt(s,pid) || pid<=0){ cout<<"Invalid ID.\n"; continue; }

        string name; double price; int stock;
        if(!fetchProductById(pid,name,price,stock)){ cout<<"No such product.\n"; continue; }
        cout<<"Selected: "<<name<<" | Price: "<<price<<" | In stock: "<<stock<<'\n';
        int qty = askInt("Quantity: ", 1);
        if(qty>stock){ cout<<"Insufficient stock.\n"; continue; }

        cart.push_back({pid,name,qty,price});
        cout<<"Added to cart.\n";
    }

    if(cart.empty()){ cout<<"Cart empty. Cancelled.\n"; return; }

    double subtotal=0;
    for(auto& it:cart) subtotal += it.line_total();

    double vatRate = askDouble("VAT % (e.g., 5): ", 0, 100);
    double discount = askDouble("Discount amount (absolute): ", 0, subtotal);

    double vatAmt = subtotal * (vatRate/100.0);
    double grand = subtotal + vatAmt - discount;

    cout<<"\n---- BILL PREVIEW ----\n";
    cout<<left<<setw(4)<<"#"<<setw(22)<<"Item"<<right<<setw(8)<<"Qty"<<setw(12)<<"Unit"<<setw(12)<<"Total"<<'\n';
    cout<<string(58,'-')<<'\n';
    int i=1;
    for(auto& it:cart)
        cout<<left<<setw(4)<<i++<<setw(22)<<it.name.substr(0,21)
            <<right<<setw(8)<<it.qty<<setw(12)<<fixed<<setprecision(2)<<it.unit_price
            <<setw(12)<<it.line_total()<<'\n';
    cout<<string(58,'-')<<'\n';
    cout<<left<<setw(34)<<"Subtotal:"<<right<<setw(24)<<fixed<<setprecision(2)<<subtotal<<'\n';
    cout<<left<<setw(34)<<"VAT:"<<right<<setw(24)<<vatAmt<<'\n';
    cout<<left<<setw(34)<<"Discount:"<<right<<setw(24)<<discount<<'\n';
    cout<<left<<setw(34)<<"Grand Total:"<<right<<setw(24)<<grand<<'\n';

    string confirm = askText("Confirm sale? (y/n): ");
    if(confirm!="y" && confirm!="Y"){ cout<<"Cancelled.\n"; return; }

    // Transaction: insert sale, sale_items, update stock
    if(!gdb.exec("BEGIN;")){ cout<<"Failed to start transaction.\n"; return; }

    // insert sale
    sqlite3_stmt* insSale=nullptr;
    const char* sqlSale="INSERT INTO sales (ts,subtotal,vat,discount,grand_total) VALUES (?,?,?,?,?);";
    if(sqlite3_prepare_v2(gdb.db, sqlSale, -1, &insSale, nullptr)!=SQLITE_OK){ gdb.exec("ROLLBACK;"); cout<<"Prepare sale failed.\n"; return; }
    sqlite3_bind_text(insSale,1, nowISO().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(insSale,2, subtotal);
    sqlite3_bind_double(insSale,3, vatAmt);
    sqlite3_bind_double(insSale,4, discount);
    sqlite3_bind_double(insSale,5, grand);
    if(sqlite3_step(insSale)!=SQLITE_DONE){ cout<<"Insert sale failed.\n"; sqlite3_finalize(insSale); gdb.exec("ROLLBACK;"); return; }
    sqlite3_finalize(insSale);

    long long sale_id = sqlite3_last_insert_rowid(gdb.db);

    // insert items + reduce stock
    sqlite3_stmt* insItem=nullptr;
    const char* sqlItem="INSERT INTO sale_items (sale_id,product_id,qty,unit_price,line_total) VALUES (?,?,?,?,?);";
    if(sqlite3_prepare_v2(gdb.db, sqlItem, -1, &insItem, nullptr)!=SQLITE_OK){ gdb.exec("ROLLBACK;"); cout<<"Prepare item failed.\n"; return; }

    bool ok=true;
    for(auto& it:cart){
        // reduce stock
        if(!updateStockDelta(it.product_id, -it.qty)){ ok=false; break; }

        sqlite3_bind_int(insItem,1,(int)sale_id);
        sqlite3_bind_int(insItem,2,it.product_id);
        sqlite3_bind_int(insItem,3,it.qty);
        sqlite3_bind_double(insItem,4,it.unit_price);
        sqlite3_bind_double(insItem,5,it.line_total());
        if(sqlite3_step(insItem)!=SQLITE_DONE){ ok=false; break; }
        sqlite3_reset(insItem);
        sqlite3_clear_bindings(insItem);
    }
    sqlite3_finalize(insItem);

    if(!ok){ gdb.exec("ROLLBACK;"); cout<<"Sale failed (stock or insert error).\n"; return; }

    gdb.exec("COMMIT;");
    cout<<"✅ Sale saved. Invoice ID: "<<sale_id<<"\n";
}

void listSales() {
    const char* sql = "SELECT id, ts, subtotal, vat, discount, grand_total FROM sales ORDER BY id DESC LIMIT 50;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ cerr<<"Query failed\n"; return; }
    cout<<left<<setw(6)<<"ID"<<setw(20)<<"Timestamp"<<right<<setw(12)<<"Subtotal"<<setw(10)<<"VAT"
        <<setw(12)<<"Discount"<<setw(14)<<"GrandTotal"<<'\n';
    cout<<string(74,'-')<<'\n';
    while(sqlite3_step(st)==SQLITE_ROW){
        int id = sqlite3_column_int(st,0);
        string ts = (const char*)sqlite3_column_text(st,1);
        double subtotal = sqlite3_column_double(st,2);
        double vat = sqlite3_column_double(st,3);
        double discount = sqlite3_column_double(st,4);
        double grand = sqlite3_column_double(st,5);
        cout<<left<<setw(6)<<id<<setw(20)<<ts<<right<<setw(12)<<fixed<<setprecision(2)<<subtotal
            <<setw(10)<<vat<<setw(12)<<discount<<setw(14)<<grand<<'\n';
    }
    sqlite3_finalize(st);
}

void viewInvoice() {
    int saleId = askInt("Enter Invoice/Sale ID: ", 1);
    // header
    sqlite3_stmt* st=nullptr;
    const char* sqlSale="SELECT ts, subtotal, vat, discount, grand_total FROM sales WHERE id=?;";
    if(sqlite3_prepare_v2(gdb.db, sqlSale, -1, &st, nullptr)!=SQLITE_OK){ cout<<"Query fail\n"; return; }
    sqlite3_bind_int(st,1,saleId);
    if(sqlite3_step(st)!=SQLITE_ROW){ cout<<"Not found.\n"; sqlite3_finalize(st); return; }
    string ts=(const char*)sqlite3_column_text(st,0);
    double subtotal=sqlite3_column_double(st,1);
    double vat=sqlite3_column_double(st,2);
    double discount=sqlite3_column_double(st,3);
    double grand=sqlite3_column_double(st,4);
    sqlite3_finalize(st);

    cout<<"\n===== INVOICE #"<<saleId<<" =====\n";
    cout<<"Date: "<<ts<<"\n";
    cout<<left<<setw(4)<<"#"<<setw(22)<<"Item"<<right<<setw(8)<<"Qty"<<setw(12)<<"Unit"<<setw(12)<<"Total"<<'\n';
    cout<<string(58,'-')<<'\n';

    const char* sqlItems =
        "SELECT p.name, si.qty, si.unit_price, si.line_total "
        "FROM sale_items si JOIN products p ON p.id=si.product_id "
        "WHERE si.sale_id=?;";
    if(sqlite3_prepare_v2(gdb.db, sqlItems, -1, &st, nullptr)!=SQLITE_OK){ cout<<"Query fail\n"; return; }
    sqlite3_bind_int(st,1,saleId);
    int i=1;
    while(sqlite3_step(st)==SQLITE_ROW){
        string name=(const char*)sqlite3_column_text(st,0);
        int qty=sqlite3_column_int(st,1);
        double unit=sqlite3_column_double(st,2);
        double total=sqlite3_column_double(st,3);
        cout<<left<<setw(4)<<i++<<setw(22)<<name.substr(0,21)
            <<right<<setw(8)<<qty<<setw(12)<<fixed<<setprecision(2)<<unit
            <<setw(12)<<total<<'\n';
    }
    sqlite3_finalize(st);

    cout<<string(58,'-')<<'\n';
    cout<<left<<setw(34)<<"Subtotal:"<<right<<setw(24)<<fixed<<setprecision(2)<<subtotal<<'\n';
    cout<<left<<setw(34)<<"VAT:"<<right<<setw(24)<<vat<<'\n';
    cout<<left<<setw(34)<<"Discount:"<<right<<setw(24)<<discount<<'\n';
    cout<<left<<setw(34)<<"Grand Total:"<<right<<setw(24)<<grand<<'\n';
}

void exportProductsCSV() {
    string path = askText("CSV file path (e.g., products.csv): ");
    FILE* f = fopen(path.c_str(), "w");
    if(!f){ cout<<"Cannot open file.\n"; return; }
    fprintf(f, "id,sku,name,category,price,stock\n");

    const char* sql = "SELECT id, IFNULL(sku,''), name, IFNULL(category,''), price, stock FROM products ORDER BY id;";
    sqlite3_stmt* st=nullptr;
    if(sqlite3_prepare_v2(gdb.db, sql, -1, &st, nullptr)!=SQLITE_OK){ fclose(f); cout<<"Query fail\n"; return; }

    while(sqlite3_step(st)==SQLITE_ROW){
        int id=sqlite3_column_int(st,0);
        string sku=(const char*)sqlite3_column_text(st,1);
        string name=(const char*)sqlite3_column_text(st,2);
        string cat=(const char*)sqlite3_column_text(st,3);
        double price=sqlite3_column_double(st,4);
        int stock=sqlite3_column_int(st,5);

        // basic CSV escaping for commas/quotes
        auto esc=[&](const string& s){
            if(s.find_first_of(",\"\n")!=string::npos){
                string t="\"";
                for(char c: s){ if(c=='\"') t+="\"\""; else t+=c; }
                t+="\""; return t;
            }
            return s;
        };

        fprintf(f, "%d,%s,%s,%s,%.2f,%d\n", id, esc(sku).c_str(), esc(name).c_str(), esc(cat).c_str(), price, stock);
    }
    sqlite3_finalize(st);
    fclose(f);
    cout<<"✅ Exported to "<<path<<'\n';
}

// --------------------------- Menu ---------------------------
void productMenu() {
    while(true){
        cout<<"\n=== Products ===\n"
            <<"1) Add product\n"
            <<"2) List products\n"
            <<"3) Search products\n"
            <<"4) Update product\n"
            <<"5) Delete product\n"
            <<"0) Back\n"
            <<"Choose: ";
        string s; getline(cin,s);
        if(s=="1") addProduct();
        else if(s=="2") listProducts();
        else if(s=="3") searchProducts();
        else if(s=="4") updateProduct();
        else if(s=="5") deleteProduct();
        else if(s=="0" || cin.eof()) break;
        else cout<<"Invalid.\n";
    }
}

void reportsMenu() {
    while(true){
        cout<<"\n=== Reports ===\n"
            <<"1) List last 50 sales\n"
            <<"2) View invoice by ID\n"
            <<"3) Export products to CSV\n"
            <<"0) Back\n"
            <<"Choose: ";
        string s; getline(cin,s);
        if(s=="1") listSales();
        else if(s=="2") viewInvoice();
        else if(s=="3") exportProductsCSV();
        else if(s=="0" || cin.eof()) break;
        else cout<<"Invalid.\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(&cout);

    if(!gdb.open("pos_inventory.db")) {
    	cerr << "[ERROR] Cannot open database in this folder.\n";  // <-- error message
    	return 1;
    }
    if(!initSchema()) {
    	cerr << "[ERROR] Failed to initialize schema.\n";          // <-- error message
    	return 1;
    }
	cout << "[INFO] Database opened and schema initialized.\n";   // <-- optional success message


    while(true){
        cout<<"\n==============================\n";
        cout<<"  Inventory & POS (SQLite)\n";
        cout<<"==============================\n";
        cout<<"1) Products\n";
        cout<<"2) New Sale (Billing)\n";
        cout<<"3) Reports\n";
        cout<<"0) Exit\n";
        cout<<"Choose: ";
        string s; getline(cin,s);
        if(s=="1") productMenu();
        else if(s=="2") newSale();
        else if(s=="3") reportsMenu();
        else if(s=="0" || cin.eof()){ cout<<"Bye!\n"; break; }
        else cout<<"Invalid choice.\n";
    }
    return 0;
}
