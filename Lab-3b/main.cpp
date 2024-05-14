#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

using namespace std;

pthread_mutex_t monitor;
pthread_cond_t red_citaca, red_pisaca, red_brisaca;

int br_citaca = 10, br_pisaca = 4, br_brisaca = 1;
int aktivni_citaci = 0, aktivni_pisaci = 0, aktivni_brisaci = 0;
int cekaju_brisaci = 0;

struct Node {
    int data;
    Node* next;
    Node(const int& newData) : data(newData), next(nullptr) {}
};
class LinkedList {
private:
    Node* head;
    int size;
public:

    LinkedList() : head(nullptr) {
        size = 0;
    }
    ~LinkedList() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    void append(const int& data) {
        size++;
        if (!head) {
            head = new Node(data);
            return;
        }
        Node* current = head;
        while (current->next) {
            current = current->next;
        }
        current->next = new Node(data);
    }

    int getSize() {
        return size;
    }

    int getElement(int index) {
        if (size == 0 || index < 0 || index >= size) {
            return -1;
        }

        Node* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }

    int remove(int index) {
        if (index < 0 || index >= size) {
            return -1;
        }
        size--;
        if (index == 0) {
            Node* temp = head;
            head = head->next;
            int data = temp->data;
            delete temp;
            return data;
        }
        Node* current = head;
        for (int i = 0; i < index - 1; i++) {
            current = current->next;
        }
        Node* temp = current->next;
        current->next = current->next->next;
        int data = temp->data;
        delete temp;
        return data;
    }

    void display() const {
        cout << "Lista: ";
        Node* current = head;
        while (current) {
            cout << current->data << " ";
            current = current->next;
        }
        cout << endl;
    }
};
LinkedList lista;

void printState() {
    cout << "aktivnih: citaca=" << aktivni_citaci << " pisaca=" << aktivni_pisaci << " brisaca=" << aktivni_brisaci << endl;
    lista.display();
    cout << endl;
}

void* citac(void* arg) {
    const int id = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    while (true) {
        pthread_mutex_lock(&monitor);
        int index = rand() % lista.getSize();
        cout << "Citac " << id << " zeli procitati " << index + 1 <<". element iz liste\n";
        printState();

        while (aktivni_brisaci > 0 || cekaju_brisaci > 0)
            pthread_cond_wait(&red_citaca, &monitor);
        aktivni_citaci++;

        int value = lista.getElement(index);

        if (value == -1) {
            cout << "Citac " << id << " nije uspio procitati " << index + 1 << ". element liste\n";
        } else {
            cout << "Citac " << id << " cita " << index + 1 << ". element s vrijednoscu " << value << endl;
        }
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);

        pthread_mutex_lock(&monitor);
        aktivni_citaci--;
        if (aktivni_citaci == 0 && cekaju_brisaci > 0)
            pthread_cond_signal(&red_brisaca);
        pthread_cond_signal(&red_citaca);

        cout << "Pisac " << id << " vise ne koristi listu\n";
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);
    }
}

void* pisac(void* arg) {
    const int id = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    while (true) {
        pthread_mutex_lock(&monitor);
        int value = rand() % 100;

        cout << "Pisac " << id << " zeli upisati vrijednost " << value << "  u listu\n";
        printState();

        while (aktivni_pisaci > 0 || aktivni_brisaci > 0 || cekaju_brisaci > 0)
            pthread_cond_wait(&red_pisaca, &monitor);
        aktivni_pisaci++;

        lista.append(value);
        cout << "Pisac "<< id <<" upisuje vrijednost " << value << endl;
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);

        pthread_mutex_lock(&monitor);
        aktivni_pisaci--;
        if (aktivni_pisaci == 0 && cekaju_brisaci > 0)
            pthread_cond_signal(&red_brisaca);
        pthread_cond_signal(&red_pisaca);

        cout << "Pisac " << id << " vise ne koristi listu\n";
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);
    }
}

void* brisac(void* arg) {
    const int id = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    while (true) {
        pthread_mutex_lock(&monitor);
        int index = rand() % lista.getSize();

        cout << "Brisac " << id << " zeli obrisati " << index + 1 << ". element iz liste\n";
        printState();

        cekaju_brisaci++;
        while (aktivni_brisaci > 0 || aktivni_citaci > 0 || aktivni_pisaci > 0)
            pthread_cond_wait(&red_brisaca, &monitor);
        cekaju_brisaci--;
        aktivni_brisaci++;

        int value = lista.remove(index);
        if (value == -1) {
            cout << "Brisac " << id << " nije uspio obrisati " << index + 1 << ". element liste\n";
        } else {
            cout << "Brisac " << id << " brise " << index + 1 << ". element s vrijednoscu " << value << endl;
        }
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);

        pthread_mutex_lock(&monitor);
        aktivni_brisaci--;
        if (aktivni_brisaci == 0) {
            pthread_cond_broadcast(&red_citaca);
            pthread_cond_broadcast(&red_pisaca);
        }

        cout << "Brisac " << id << " vise ne koristi listu\n";
        printState();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 6 + 5);
    }
}

int main() {
    srand(time(0));

    pthread_t citaci[br_citaca], pisaci[br_pisaca], brisaci[br_brisaca];

    pthread_mutex_init(&monitor, nullptr);
    pthread_cond_init(&red_citaca, nullptr);
    pthread_cond_init(&red_pisaca, nullptr);
    pthread_cond_init(&red_brisaca, nullptr);

    for (int i = 0; i < br_pisaca; i++)
        pthread_create(&pisaci[i], nullptr, pisac, (void*) i);

    sleep(10);

    for (int i = 0; i < br_citaca; i++)
        pthread_create(&citaci[i], nullptr, citac, (void*) i);

    for (int i = 0; i < br_brisaca; i++)
        pthread_create(&brisaci[i], nullptr, brisac, (void *) i);

    for (int i = 0; i < br_citaca; i++)
        pthread_join(citaci[i], nullptr);
    for (int i = 0; i < br_pisaca; i++)
        pthread_join(pisaci[i], nullptr);
    for (int i = 0; i < br_brisaca; i++)
        pthread_join(brisaci[i], nullptr);

    pthread_mutex_destroy(&monitor);
    pthread_cond_destroy(&red_citaca);
    pthread_cond_destroy(&red_pisaca);
    pthread_cond_destroy(&red_brisaca);;

    return 0;
}