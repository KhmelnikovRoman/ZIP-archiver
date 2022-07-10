#include <iostream>
#include <string>
#include <vector>
#include <clocale>

using namespace std;

class Zipper {

private:
    char temp = 92;
    vector<string> files;   // набор файлов (-files)
    string path;            // путь (-path)    
    string real_bin_file;   // имя выходного файла-архива( используется при архивации )
public:
    Zipper(vector<string>& vec, string p)
    {
        if (vec.size() > 0) files.assign(vec.begin(), vec.end());
        path = p + temp;
        real_bin_file = path + "binary.zipper";
    }

    void getInfo();   // Метод для получения информации о файлах на этапе архивации
    void InCompress();   // Архивация данных
    void OutCompress(string binary);   // Распаковка данных ( binary - путь до архива )

    // Статический метод для выделения имени файла из полного пути. 
    // Используется для внутренних нужд.
    static string get_file_name(string fn) {
        char temp = 92;
        return fn.substr(fn.find_last_of(temp) + 1, fn.size()); 
    }
};

int digs(double w)
{
    int yield = 0;
    while (w > 10) { yield++; w /= 10; }
    return yield + 1;
}

void Zipper::getInfo()
{
    char byte[1];  // единичный буфер для считывания одного байта

    basic_string<char> s_info = "";
    remove((this->path + "info.txt").c_str());  // на всякий случай
    FILE* info = fopen((this->path + "info.txt").c_str(), "a+");  // сохраняем информацию в наш текстовый файл
    int bytes_size = 0;  // длина информационного блока в байтах
    for (vector<string>::iterator itr = this->files.begin() + 1; itr != this->files.end(); ++itr)
    {
        FILE* f = fopen((*itr).c_str(), "rb");
        if (f) {

            // получаем размер архивируемого файла
            fseek(f, 0, SEEK_END);
            int size = ftell(f);

            string name = Zipper::get_file_name(*itr);  // получаем имя архивируемого файла

            char* m_size = new char[digs(size) + 1];
            _itoa(size, m_size, 10);
            fclose(f);

            bytes_size += digs(size);
            bytes_size += strlen(name.c_str());

            // все, что "нарыли", сохраняем в промежуточный буфер :
            s_info.append(m_size);
            s_info.append("||");
            s_info.append(name);
            s_info.append("||");

            delete[] m_size;
        }

    }
    bytes_size = s_info.size() + 2;
    char* b_buff = new char[digs(bytes_size)];
    _itoa(bytes_size, b_buff, 10);

    // форматируем до 5 байт
    if (digs(bytes_size) < 5) fputs(string(5 - digs(bytes_size), '0').c_str(), info);

    fputs(b_buff, info);
    fputs("||", info);
    fputs(s_info.c_str(), info);

    fclose(info);
}


void Zipper::InCompress()
{
    char byte[1];  // единичный буфер для считывания одного байта

    getInfo();  // получаем необходимую информацию о том, что архивируем

    FILE* f;
    FILE* main = fopen((this->real_bin_file).c_str(), "wb");  // файл - архив
    FILE* info = fopen((this->path + "info.txt").c_str(), "rb");  // файл с информацией

    // переписываем информацию в архив
    while (!feof(info))
    {
        if (fread(byte, 1, 1, info) == 1) fwrite(byte, 1, 1, main);
    }

    fclose(info);
    remove((this->path + "info.txt").c_str());  // прибираемся за собой

    // последовательная запись в архив архивируемых файлов побайтно :
    for (vector<string>::iterator itr = this->files.begin() + 1; itr != this->files.end(); ++itr)
    {
        f = fopen((*itr).c_str(), "rb");
        if (!f) { cout << *itr << " не найден!" << endl; system("pause"); }
        else {
            while (!feof(f))
            {
                if (fread(byte, 1, 1, f) == 1) fwrite(byte, 1, 1, main);
            }
            cout << *itr << " добавлен в архив '" << this->real_bin_file << "'." << endl;
            fclose(f);
        }
    }
    system("pause");
    fclose(main);
}

void Zipper::OutCompress(string binary)
{
    FILE* bin = fopen(binary.c_str(), "rb");   // открываем архив в режиме чтения
    if (bin) {
        cout << binary << " is open.\n";
        char info_block_size[5];   // размер информационного блока 
        fread(info_block_size, 1, 5, bin);  // получаем размер
        int _sz = atoi(info_block_size);  // преобразуем буфер в число

        char* info_block = new char[_sz];  // информационный блок
        fread(info_block, 1, _sz, bin);   // считываем его 

        // Парсинг информационного блока :
        vector<string> tokens;
        char* tok = strtok(info_block, "||");
        int toks = 0;
        while (tok)
        {
            if (strlen(tok) == 0) break;
            tokens.push_back(tok);
            tok = strtok(NULL, "||");
            toks++;
        }

        if (toks % 2 == 1) toks--;  // удаляем мусор
        int files = toks / 2;  // количество обнаруженных файлов в архиве

        char byte[1];   // единичный буфер для считывания одного байта

        // Процесс распаковки всех файлов( по правилам полученным из блока с информацией ) :
        for (int i = 0; i < files; i++)
        {
            const char* size = tokens[i * 2].c_str();
            const char* name = tokens[i * 2 + 1].c_str();
            char full_path[255];
            strcpy(full_path, this->path.c_str());
            strcat(full_path, name);
            int _sz = atoi(size);
            cout << "--  '" << name << "' извлечен в '" << this->path << "' ." << endl;
            FILE* curr = fopen(full_path, "wb");
            for (int r = 1; r <= _sz; r++)
            {
                if (fread(byte, 1, 1, bin) == 1) fwrite(byte, 1, 1, curr);
            }
            fclose(curr);
        }
        system("pause");
        fclose(bin);
    }
}

int getInt()
{
    while (true) {
        int a;
        cin >> a;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(32767, '\n');
        }
        else
            return a;
    }
}

int main(int argv, char* argc[])
{
    vector<string> files;  // массив файлов, переданных через параметры из консоли
    string path = "";  // путь
    string pathR = "";
    string tempA = "";
    string tempR = "";
    bool flag_fs = false, flag_path = false;  // флаги режима чтения/записи
    char type[6];              // тип: упаковка или распаковка 
    memset(type, 0, 6);
    bool tf = true;
    int choice = 0;

    setlocale(LC_ALL, "Russian");
    while (tf) {
        system("cls");
        cout << "######################## ZIPPER ########################" << endl;
        cout << "1) Архивирование\n" << "2) Разархивирование\n" << "3) Выход\n" << "Выберете действие: ";
        choice = getInt();
        if (choice == 1) {
            system("cls");
            cout << "Введите пути к файлам для архивирования через Enter (для завершения ввода нажмите .):\n";
            tempA = "";
            getline(cin, tempA);
            while (tempA != ".") {
                files.push_back(string(tempA));
                getline(cin, tempA);
            }
            cout << "Введите путь хранения архива:\n";
            getline(cin, path);
            Zipper* zipA = new Zipper(files, path);
            zipA->InCompress();
            delete(zipA);
        }
        else if (choice == 2) {
            system("cls");
            cout << "Введите пути к файлам для разархивирования через Enter (для завершения ввода нажмите .):\n";
            tempR = "";
            while (tempR != ".") {
                files.push_back(string(tempR));
                getline(cin, tempR);
            }
            cout << "Введите путь хранения архива:\n";
            getline(cin, pathR);
            Zipper* zipR = new Zipper(files, pathR);
            zipR->OutCompress(files[1]);
            delete(zipR);
        }
        else if (choice == 3) {
            tf = false;
        }
        else {
            system("cls");
        }
    }
}
