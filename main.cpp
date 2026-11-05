#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <cstdlib>
#include "db.h"

const string BACKUP_FILE = "warehouse_backup.db";



vector<Record> g_displayData;

class RecordTable : public Fl_Table_Row {
public:
    RecordTable(int X, int Y, int W, int H) : Fl_Table_Row(X, Y, W, H) {
        rows(0);
        cols(5);
        col_header(1);
        col_width_all(100);
        row_header(0);
        end();
    }

    void draw_cell(TableContext context, int R = 0, int C = 0,
                   int X = 0, int Y = 0, int W = 0, int H = 0) override {
        static const char* headers[] = {"ID", "Название", "Кол-во", "Цена", "Категория"};

        switch (context) {
            case CONTEXT_STARTPAGE:
                fl_font(FL_HELVETICA, 14);
                return;

            case CONTEXT_COL_HEADER:
                fl_font(FL_HELVETICA, 14);
                fl_push_clip(X, Y, W, H);
                fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_GRAY);
                fl_color(FL_BLACK);
                fl_draw(headers[C], X, Y, W, H, FL_ALIGN_CENTER);
                fl_pop_clip();
                return;

            case CONTEXT_CELL: {
                fl_font(FL_HELVETICA, 14);
                fl_push_clip(X, Y, W, H);
                fl_color(FL_WHITE);
                fl_rectf(X, Y, W, H);
                fl_color(FL_BLACK);

                if (R >= 0 && R < (int)g_displayData.size()) {
                    char buf[64] = "";
                    const Record& rec = g_displayData[R];
                    switch (C) {
                        case 0: snprintf(buf, sizeof(buf), "%d", rec.id); break;
                        case 1: snprintf(buf, sizeof(buf), "%s", rec.name); break;
                        case 2: snprintf(buf, sizeof(buf), "%d", rec.quantity); break;
                        case 3: snprintf(buf, sizeof(buf), "%.2f", rec.price); break;
                        case 4: snprintf(buf, sizeof(buf), "%c", rec.category); break;
                    }
                    fl_draw(buf, X, Y, W, H, FL_ALIGN_CENTER);
                }
                fl_color(FL_GRAY);
                fl_rect(X, Y, W, H);
                fl_pop_clip();
                return;
            }

            default:
                return;
        }
    }
};


RecordTable*    g_table    = nullptr;
Fl_Int_Input*   g_inId     = nullptr;
Fl_Input*       g_inName   = nullptr;
Fl_Int_Input*   g_inQty    = nullptr;
Fl_Float_Input* g_inPrice  = nullptr;
Fl_Input*       g_inCat    = nullptr;


void refreshTable() {
    g_displayData.clear();
    for (auto& pair : g_index) {
        Record r;
        if (findByKey(DB_FILE, pair.first, r)) {
            g_displayData.push_back(r);
        }
    }
    g_table->rows((int)g_displayData.size());
    g_table->redraw();
}

void showOnlyOne(const Record& r) {
    g_displayData.clear();
    g_displayData.push_back(r);
    g_table->rows(1);
    g_table->redraw();
}

void showList(const vector<Record>& list) {
    g_displayData = list;
    g_table->rows((int)g_displayData.size());
    g_table->redraw();
}

void clearInputs() {
    g_inId->value("");
    g_inName->value("");
    g_inQty->value("");
    g_inPrice->value("");
    g_inCat->value("");
}

bool getIdFromInput(int& outId) {
    if (strlen(g_inId->value()) == 0) {
        fl_alert("Введите ID");
        return false;
    }
    outId = atoi(g_inId->value());
    return true;
}

bool getCategoryFromInput(char& outCat) {
    if (strlen(g_inCat->value()) == 0) {
        fl_alert("Введите категорию");
        return false;
    }
    outCat = g_inCat->value()[0];
    return true;
}


void onAdd(Fl_Widget*, void*) {
    if (strlen(g_inId->value()) == 0)   { fl_alert("Введите ID (ключевое поле)"); return; }
    if (strlen(g_inName->value()) == 0) { fl_alert("Введите название товара"); return; }
    if (strlen(g_inCat->value()) == 0)  { fl_alert("Введите категорию (одна буква)"); return; }

    Record r;
    r.id = atoi(g_inId->value());
    strncpy(r.name, g_inName->value(), sizeof(r.name) - 1);
    r.name[sizeof(r.name) - 1] = '\0';
    r.quantity = atoi(g_inQty->value());
    r.price = (float)atof(g_inPrice->value());
    r.category = g_inCat->value()[0];
    r.deleted = false;
    r.next_free = -1;

    if (addRecord(DB_FILE, r)) {
        clearInputs();
        refreshTable();
    } else {
        fl_alert("Не удалось добавить: запись с таким ID уже существует");
    }
}

void onFindById(Fl_Widget*, void*) {
    int id;
    if (!getIdFromInput(id)) return;

    Record r;
    if (findByKey(DB_FILE, id, r)) {
        showOnlyOne(r);
    } else {
        fl_alert("Запись с таким ID не найдена");
    }
}

void onFindByCategory(Fl_Widget*, void*) {
    char cat;
    if (!getCategoryFromInput(cat)) return;

    vector<Record> results = findByCategory(DB_FILE, cat);
    if (results.empty()) {
        fl_alert("Ничего не найдено по этой категории");
    } else {
        showList(results);
    }
}

void onShowAll(Fl_Widget*, void*) {
    refreshTable();
}

void onDeleteById(Fl_Widget*, void*) {
    int id;
    if (!getIdFromInput(id)) return;

    if (deleteByKey(DB_FILE, id)) {
        refreshTable();
    } else {
        fl_alert("Запись с таким ID не найдена");
    }
}

void onDeleteByCategory(Fl_Widget*, void*) {
    char cat;
    if (!getCategoryFromInput(cat)) return;

    int count = deleteByCategory(DB_FILE, cat);
    refreshTable();

    char msg[64];
    snprintf(msg, sizeof(msg), "Удалено записей: %d", count);
    fl_message("%s", msg);
}

void onEdit(Fl_Widget*, void*) {
    int id;
    if (!getIdFromInput(id)) return;
    if (strlen(g_inName->value()) == 0) { fl_alert("Введите новое название"); return; }
    if (strlen(g_inCat->value()) == 0)  { fl_alert("Введите категорию"); return; }

    Record updated;
    updated.id = id;
    strncpy(updated.name, g_inName->value(), sizeof(updated.name) - 1);
    updated.name[sizeof(updated.name) - 1] = '\0';
    updated.quantity = atoi(g_inQty->value());
    updated.price = (float)atof(g_inPrice->value());
    updated.category = g_inCat->value()[0];
    updated.deleted = false;
    updated.next_free = -1;

    if (editRecord(DB_FILE, id, updated)) {
        clearInputs();
        refreshTable();
    } else {
        fl_alert("Запись с таким ID не найдена");
    }
}


void onCreateDb(Fl_Widget*, void*) {
    int choice = fl_choice("Создать новую БД? Все текущие данные будут СТЕРТЫ.", "Отмена", "Создать", nullptr);
    if (choice == 1) {
        createDatabase(DB_FILE);
        g_index.clear();
        refreshTable();
    }
}

void onOpenDb(Fl_Widget*, void*) {
    if (openDatabase(DB_FILE)) {
        refreshTable();
    } else {
        fl_alert("Не удалось открыть БД (файл не найден)");
    }
}

void onDeleteDb(Fl_Widget*, void*) {
    int choice = fl_choice("Удалить файл БД навсегда?", "Отмена", "Удалить", nullptr);
    if (choice == 1) {
        if (deleteDatabase(DB_FILE)) {
            g_displayData.clear();
            g_table->rows(0);
            g_table->redraw();
            fl_message("БД удалена");
        } else {
            fl_alert("Не удалось удалить файл");
        }
    }
}

void onClearDb(Fl_Widget*, void*) {
    int choice = fl_choice("Очистить БД? Все записи будут удалены (файл останется).", "Отмена", "Очистить", nullptr);
    if (choice == 1) {
        clearDatabase(DB_FILE);
        refreshTable();
    }
}

void onBackup(Fl_Widget*, void*) {
    if (backupDatabase(DB_FILE, BACKUP_FILE)) {
        fl_message("Backup создан: %s", BACKUP_FILE.c_str());
    } else {
        fl_alert("Не удалось создать backup");
    }
}

void onRestore(Fl_Widget*, void*) {
    int choice = fl_choice("Восстановить БД из backup'а? Текущие данные будут заменены.", "Отмена", "Восстановить", nullptr);
    if (choice == 1) {
        if (restoreDatabase(DB_FILE, BACKUP_FILE)) {
            refreshTable();
            fl_message("БД восстановлена из backup'а");
        } else {
            fl_alert("Не удалось восстановить (backup не найден?)");
        }
    }
}


void onImportCsv(Fl_Widget*, void*) {
    const char* path = fl_input("Введите путь к CSV файлу:", "import.csv");
    if (!path) return;

    int count = importFromCSV(DB_FILE, path);
    if (count < 0) {
        fl_alert("Не удалось открыть CSV файл");
    } else {
        fl_message("Импортировано записей: %d", count);
        refreshTable();
    }
}


int main() {
    ifstream check(DB_FILE);
    if (check.good()) {
        check.close();
        openDatabase(DB_FILE);
    } else {
        createDatabase(DB_FILE);
    }

    Fl_Window* win = new Fl_Window(900, 620, "Склад - база данных");

    g_table = new RecordTable(10, 10, 880, 270);


    g_inId    = new Fl_Int_Input(130, 295, 180, 25, "ID:");
    g_inName  = new Fl_Input(130, 325, 180, 25, "Название:");
    g_inQty   = new Fl_Int_Input(130, 355, 180, 25, "Количество:");
    g_inPrice = new Fl_Float_Input(130, 385, 180, 25, "Цена:");
    g_inCat   = new Fl_Input(130, 415, 180, 25, "Категория:");

    Fl_Button* btnFindId    = new Fl_Button(380, 295, 180, 25, "Найти по ID");
    Fl_Button* btnFindCat   = new Fl_Button(380, 325, 180, 25, "Найти по категории");
    Fl_Button* btnDelId     = new Fl_Button(380, 355, 180, 25, "Удалить по ID");
    Fl_Button* btnDelCat    = new Fl_Button(380, 385, 180, 25, "Удалить по категории");
    Fl_Button* btnShowAll   = new Fl_Button(380, 415, 180, 25, "Показать все");

    btnFindId->callback(onFindById);
    btnFindCat->callback(onFindByCategory);
    btnDelId->callback(onDeleteById);
    btnDelCat->callback(onDeleteByCategory);
    btnShowAll->callback(onShowAll);

    Fl_Button* btnAdd  = new Fl_Button(130, 455, 180, 30, "Добавить");
    Fl_Button* btnEdit = new Fl_Button(380, 455, 180, 30, "Редактировать (по ID)");
    btnAdd->callback(onAdd);
    btnEdit->callback(onEdit);

    Fl_Button* btnCreate  = new Fl_Button(10,  505, 140, 30, "Создать БД");
    Fl_Button* btnOpen    = new Fl_Button(160, 505, 140, 30, "Открыть БД");
    Fl_Button* btnDeleteDb= new Fl_Button(310, 505, 140, 30, "Удалить БД");
    Fl_Button* btnClear   = new Fl_Button(460, 505, 140, 30, "Очистить БД");
    Fl_Button* btnBackup  = new Fl_Button(610, 505, 130, 30, "Backup");
    Fl_Button* btnRestore = new Fl_Button(750, 505, 130, 30, "Restore");

    btnCreate->callback(onCreateDb);
    btnOpen->callback(onOpenDb);
    btnDeleteDb->callback(onDeleteDb);
    btnClear->callback(onClearDb);
    btnBackup->callback(onBackup);
    btnRestore->callback(onRestore);

    Fl_Button* btnImportCsv = new Fl_Button(10, 545, 200, 30, "Импорт из CSV");
    btnImportCsv->callback(onImportCsv);

    win->end();
    win->show();

    refreshTable();

    return Fl::run();
}
