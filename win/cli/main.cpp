#include <windows.h>
#include "SerialPortManager.h"


/*
@TODO
* Бага: иногда принимаемое значение больше ожидаемого максимального (1000)
* Бага: иногда прием значений как будто фризит
* Программирование кнопок в опциях вызова(?)
*/
int main(unsigned int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SerialPortManager::run(argc, argv);
    return 0;
}
