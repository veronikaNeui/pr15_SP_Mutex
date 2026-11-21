// pr15_Mutex.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>

using namespace std;

struct BankAccount {
    int accountNumber;    // номер банковского счета
    int balance;          // текущий баланс
};

void writeLog(int pid, int startBalance, string type, int sum, int endBalance, string status)
{
    ofstream logfile("operations.log", ios::app);
    if (logfile.is_open())
    {
        logfile << "PID " << pid << " | Начальный баланс: " << startBalance << " | Пополнение/Снятие: " << type << " | Сумма операции: " << sum << " | Конечный баланс: " << endBalance << " | Статус: " << status << "\n";
        logfile.close();
    }
}

// чтение из бин файла
bool readAccountbin(BankAccount& account)
{
    ifstream file("account.bin", ios::binary);
    if (!file.is_open())
    {
        cout << "Не удалось открыть файл\n";
        return false;
    }
    file.read((char*)&account, sizeof(BankAccount));
    file.close();
    return true;
}

// запись счета в бин файлл
void saveAccountbin(const BankAccount& account)
{
    ofstream file("account.bin", ios::binary);
    if (file.is_open())
    {
        file.seekp(0);
        file.write((char*)&account, sizeof(BankAccount));
        file.close();
    }
}

void initFile()
{
    BankAccount account;
    if (!readAccountbin(account))
    {
        account.accountNumber = 10001;
        account.balance = 10000;
        saveAccountbin(account);
        cout << "Создан новый счет: " << account.accountNumber << " с балансом " << account.balance << " руб. \n";

    }
}

int main()
{
    setlocale(LC_ALL, "Russian");
    srand(time(NULL));

    int pid = GetCurrentProcessId();
    initFile();

    HANDLE hMutex = CreateMutex(NULL, FALSE, L"Local\\NeuiminaMutex");
    if (hMutex == NULL)
    {
        cout << "Ошибка создания мьютекса" << endl;
        return 1;
    }

    int choice;

    while (true)
    {
        cout << "1.Просмотр текущего баланса банковского счета\n2.Пополнение счета на указанную сумму\n3.Снятие денежных средств\n4.Автовыполнение 20 случайных операций пополнения и снятия денежных средств на суммы от 100 до 1000 руб.\n5.Выход\nВаш выбор: ";
        cin >> choice;

        if (choice == 5)
        {
            break;
        }

        if (choice < 1 || choice > 5)
        {
            cout << "Неверный ввод\n";
            continue;
        }

        DWORD waitResult = WaitForSingleObject(hMutex, 3000);

        if (waitResult == WAIT_ABANDONED)
        {
            cout << "Ошибка: забытый мьютекс";
        }
        else if (waitResult != WAIT_OBJECT_0)
        {
            cout << "Ошибка: не удалось получить доступ к счету";
            continue;
        }

        BankAccount account;
        if (!readAccountbin(account))
        {
            cout << "Ошибка чтения файла\n";

        }
        else {
            if (choice == 1)
            {
                cout << "Номер счета: " << account.accountNumber << "\n";
                cout << "Текущий баланс: " << account.balance << " руб.\n";
            }
            else if (choice == 2)
            {
                int sum;
                cout << "Введите сумму для пополнения: ";
                cin >> sum;

                // сумма больше 0 и нет переполнения int 
                if (sum > 0 && (account.balance + sum) > account.balance)
                {
                    int startBalance = account.balance;
                    account.balance += sum;
                    saveAccountbin(account);
                    cout << "Успешно пополнено на " << sum << " руб. Новый баланс: " << account.balance << "\n";
                    writeLog(pid, startBalance, "Пополнение", sum, account.balance, "Успешно");

                }
                else {
                    cout << "Ошибка: сумма должна быть положительной и не вызывать переполнение\n";
                    writeLog(pid, account.balance, "Пополнение", sum, account.balance, "Ошибка");

                }
            }
            else if (choice == 3)
            {
                int sum;
                cout << "Введите сумму для снятия: ";
                cin >> sum;

                if (sum > 0 && account.balance >= sum)
                {
                    int startBalance = account.balance;
                    account.balance -= sum;
                    saveAccountbin(account);
                    cout << "Успешно снято на " << sum << " руб. Новый баланс: " << account.balance << "\n";
                    writeLog(pid, startBalance, "Снятие    ", sum, account.balance, "Успешно");

                }
                else {
                    cout << "Ошибка: недостаточно средств для снятия\n";
                    writeLog(pid, account.balance, "Снятие    ", sum, account.balance, "Ошибка"); 

                }

            }
            else if (choice == 4)
            {
                cout << "Запуск 20 случ операций\n";
                for (int i = 0; i < 20; i++)
                {
                    Sleep(100 + rand() % 401);

                    // Перечитываем данные другой процесс мог изменить файл
                    if (!readAccountbin(account))
                    {
                        break;
                    }

                    int sum = 100 + rand() % 901;
                    bool isDeposit = rand() % 2;
                    int startBal = account.balance;
                    string type = isDeposit ? "Пополнение" : "Снятие";

                    if (isDeposit) 
                    {
                        if ((account.balance + sum) > account.balance) 
                        {
                            account.balance += sum;
                            saveAccountbin(account);
                            cout << "[Авто] Пополнение " << sum << ". Баланс: " << account.balance << endl;
                            writeLog(pid, startBal, type, sum, account.balance, "Успешно");
                        }
                    }
                    else
                    {
                        if (account.balance >= sum)
                        {
                            account.balance -= sum;
                            saveAccountbin(account);
                            cout << "[Авто] Снятие " << sum << ". Баланс: " << account.balance << endl;
                            writeLog(pid, startBal, type, sum, account.balance, "Успешно");
                        }
                        else {
                            cout << "[Авто] Ошибка снятия (мало средств). Баланс: " << account.balance << endl;
                            writeLog(pid, startBal, type, sum, account.balance, "Ошибка (мало средств)");
                        }
                    }
                }
            }

            

        }
        
        ReleaseMutex(hMutex);       

    }

    CloseHandle(hMutex);
    return 0;

}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
