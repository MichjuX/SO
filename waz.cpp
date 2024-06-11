#include <iostream>
#include "windows.h"
#include <conio.h>

void gotoxy(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void ramka() {
	for (int i = 0; i <= 61; i++) {
		gotoxy(i, 0); printf("-");
		gotoxy(i, 21); printf("-");
	}
	for (int i = 0; i <= 21; i++) {
		gotoxy(0, i); printf("|");
		gotoxy(61, i); printf("|");
	}
}

void ini(char ek[80][20]) {
	int x, y;
	for (int i = 1; i < 2; i++) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		ek[x][y] = 'R';
	}
	for (int i = 1; i < 2; i++) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		ek[x][y] = 'P';
	}
}

void jedzenie(char ek[80][20]) {
	int x, y;
	for (int i = 1; i < 2; i++) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		ek[x][y] = 'R';
		gotoxy(x, y); printf("R");
	}
	for (int i = 1; i < 2; i++) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		ek[x][y] = 'P';
		gotoxy(x, y); printf("P");
	}
	for (int i = 1; i < 1; i++) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		ek[x][y] = 'X';
		gotoxy(x, y); printf("X");
	}
	if (rand() % 2) {
		x = rand() % 55 + 2;
		y = rand() % 17 + 2;
		for (int i = 1; i < 4; i++) {
			ek[x][y + i] = 'U';
			gotoxy(x, y + i); printf("U");
		}
	}
}

void druk_e(char ek[80][20]) {
	for (int i = 0; i < 20; i++)
		for (int j = 0; j < 80; j++) {
			gotoxy(j, i); printf("%c", ek[j][i]);
		}
}

void kla(int* kier, int* kier2, bool* koniec) {
	if (_kbhit()) {
		unsigned char znak = _getch();
		switch (znak) {
		case 224: // special keys
			znak = _getch();
			switch (znak) {
			case 72: // arrow up
				if (*kier != 3) *kier = 1;
				break;
			case 80: // arrow down
				if (*kier != 1) *kier = 3;
				break;
			case 75: // arrow left
				if (*kier != 2) *kier = 4;
				break;
			case 77: // arrow right
				if (*kier != 4) *kier = 2;
				break;
			}
			break;
		case 'w': // W key
		case 'W':
			if (*kier2 != 3) *kier2 = 1;
			break;
		case 's': // S key
		case 'S':
			if (*kier2 != 1) *kier2 = 3;
			break;
		case 'a': // A key
		case 'A':
			if (*kier2 != 2) *kier2 = 4;
			break;
		case 'd': // D key
		case 'D':
			if (*kier2 != 4) *kier2 = 2;
			break;
		case 13: // ENTER
			break;
		case 27: // ESC
			*koniec = true;
			break;
		}
	}
}

struct waz {
	int x, y;
	waz* nastepna;    // wskaźnik na następny element
	waz();            // konstruktor
};

waz::waz() {
	nastepna = 0;       // konstruktor
	x = 1; y = 1;
}

struct lista {
	waz* pierwsza;  // wskaźnik na początek listy
	void dodaj(int x, int y);
	void usun();
	void wyswietl();
	void czysc();
	int licz();
	int ruch(int k, char ek[80][20]);
	void ws(int* x, int* y);
	int kolizja_sciana();
	int kolizja_waz();
	lista();
};

lista::lista() {
	pierwsza = 0;       // konstruktor
}

void lista::dodaj(int x, int y) {
	waz* nowa = new waz;    // tworzy nowy element listy
	nowa->x = x;
	nowa->y = y;
	if (pierwsza == 0) { // sprawdzamy czy to pierwszy element listy
		pierwsza = nowa;
	}
	else {
		waz* temp = pierwsza;
		while (temp->nastepna) {
			temp = temp->nastepna;
		}
		temp->nastepna = nowa;  // ostatni element wskazuje na nasz nowy
		nowa->nastepna = 0;     // ostatni nie wskazuje na nic
	}
}

void lista::wyswietl() {
	waz* temp = pierwsza;
	while (temp) {
		gotoxy(temp->x, temp->y); printf("o");
		temp = temp->nastepna;
	}
}

int lista::licz() {
	waz* temp = pierwsza;
	int k = 0;
	while (temp) {
		gotoxy(temp->x, temp->y); printf("o");
		temp = temp->nastepna;
		k = k + 1;
	}
	return k;
}

int lista::kolizja_sciana() {
	waz* temp = pierwsza;
	int kz = 0;
	if ((temp->x < 2) || (temp->x > 59)) kz = 1;
	if ((temp->y < 2) || (temp->y > 19)) kz = 1;
	gotoxy(70, 10); printf("%i", kz);
	return kz;
}

int lista::kolizja_waz() {
	waz* temp = pierwsza;
	int x, y, kz = 0;
	x = temp->x; y = temp->y;
	temp = temp->nastepna;
	while (temp) {
		if ((temp->x == x) && (temp->y == y)) kz = 1;
		temp = temp->nastepna;
	}
	return kz;
}

void lista::czysc() {
	waz* temp = pierwsza;
	while (temp) {
		gotoxy(temp->x, temp->y); printf(" ");
		temp = temp->nastepna;
	}
}

void lista::usun() {
	waz* temp = pierwsza;
	while (temp->nastepna->nastepna) {
		temp = temp->nastepna;
	}
	temp->nastepna = 0;
}

int lista::ruch(int k, char ek[80][20]) {
	int x1, y1, x2, y2;
	waz* temp = pierwsza;
	x1 = temp->x; y1 = temp->y;
	switch (k) {
	case 1: temp->y = temp->y - 1; break;
	case 2: temp->x = temp->x + 1; break;
	case 3: temp->y = temp->y + 1; break;
	case 4: temp->x = temp->x - 1; break;
	}
	int kz = 0;
	if (ek[temp->x][temp->y] == 'R') kz = 1;
	if (ek[temp->x][temp->y] == 'P') kz = 2;
	if (ek[temp->x][temp->y] == 'X') kz = 3;
	if (ek[temp->x][temp->y] == 'U') kz = 4;
	ek[temp->x][temp->y] = ' ';
	temp = temp->nastepna;

	while (temp) {
		x2 = temp->x; y2 = temp->y;
		temp->x = x1; temp->y = y1;
		x1 = x2;      y1 = y2;
		temp = temp->nastepna;
	}
	if (kz == 1) {
		dodaj(x2, y2); jedzenie(ek);
	}
	if (kz == 2) {
		usun(); jedzenie(ek);
	}
	if (kz == 3) {
		dodaj(x2, y2);
		if (k == 1) dodaj(x2, y2 + 1);
		if (k == 2) dodaj(x2 - 1, y2);
		if (k == 3) dodaj(x2, y2 - 1);
		if (k == 4) dodaj(x2 + 1, y2);
		jedzenie(ek);
	}
	if (kz == 4) {
		return 1;
	}
	return 0;
}

void lista::ws(int* x, int* y) {
	waz* temp = pierwsza;
	*x = temp->x; *y = temp->y;
}

int kolizja_w(int x, int y, lista* waz2) {
	waz* temp = waz2->pierwsza;
	while (temp) {
		if ((temp->x == x) && (temp->y == y))  return 1;
		temp = temp->nastepna;
	}
	return 0;
}

int automat(int xx, int yy, int kier2, int vp, int hp)
{
	static int kkx = 0, kky1 = 0, kky2 = 0;

	if ((xx < 5) && (kkx == 0)) {
		kier2 = 3; kkx = 1; kky1 = 0;
	}
	if ((yy > vp) && (kky1 == 0)) {
		kier2 = 2; kky1 = 1; kkx = 0;
	}
	if ((xx > hp) && (kkx == 0)) {
		kier2 = 1; kkx = 1; kky2 = 0;
	}
	if ((yy < 5) && (kky2 == 0)) {
		kier2 = 4; kky2 = 1; kkx = 0;
	}
	return kier2;
}

int gra() {
	int kier = 1, kier2 = 4, xx, yy, x, y;
	int vp = 19, hp = 30;
	bool koniec = false;
	char ek[80][20] = { ' ' };
	lista* waz1 = new lista;    // tworzymy liste
	lista* waz2 = new lista;    // tworzymy liste

	ini(ek); druk_e(ek); ramka();
	waz1->dodaj(10, 10);
	waz1->dodaj(11, 10);
	waz1->dodaj(12, 10);
	waz1->dodaj(13, 10);

	waz2->dodaj(10, 13);
	waz2->dodaj(11, 13);
	waz2->dodaj(12, 13);
	waz2->dodaj(13, 13);

	waz1->wyswietl(); waz2->wyswietl();
	while (koniec == false) {
		Sleep(200);
		kla(&kier, &kier2, &koniec);
		waz1->czysc();
		if (waz1->ruch(kier, ek)) koniec = true;
		waz2->czysc(); waz2->ws(&xx, &yy);

		if ((xx < 5) && (yy < 5)) {
			vp = 7 + rand() % 10;
			hp = 10 + rand() % 40;
		}
		kier2 = automat(xx, yy, kier2, vp, hp);

		waz2->ruch(kier2, ek);
		if (waz1->kolizja_sciana()) koniec = true;
		if (waz1->kolizja_waz()) koniec = true;
		waz1->ws(&x, &y);
		if (kolizja_w(x, y, waz2) == 1) koniec = true;
		waz2->ws(&x, &y);
		if (kolizja_w(x, y, waz1) == 1) koniec = true;
		if (waz1->licz() < 3) koniec = true;
		waz1->wyswietl(); waz2->wyswietl();
	}
	return waz1->licz() - waz2->licz();
	delete (waz1); delete (waz2);
}

int main() {
	system("CLS");
	int k = 0;
	k = gra();
	system("CLS");
	gotoxy(10, 10); printf("%i", k);

	_getch();
}
