# Śpiący fryzjerzy-kasjerzy

### SPOSÓB ROZWIĄZANIA PROBLEMU

Przedstawienie obiektów IPC użytych w problemie:

- counter(kasa)-pamięć współdzielona: 
  - jest to lista 4 elementowa, w której na 3 pierwszych miejscach znajduję się liczba odpowiednich banknotów. Na ostatnim miejscu znajduje się łączna suma pieniędzy w kasie.

  - do mechanizmu kasy dodatkowo używane są 2 semafory, jeden do odczytu, drugi do zapisu


- wr(poczekalnia) - kolejka komunikatów:
  - klient wysyła za pomocą niej swoje pieniądze oraz swój identyfikator

  - fryzjer dzięki niej bierze klienta na fotel i później przy jej użyciu oraz wiadomości odebranej wysyla wiadomość o zakończeniu usługi to odpowiedniego klienta, wraz z tą wiadomością, zwracane są pozostałe pieniądze klienta wraz z resztą za usługę

- num_clients(liczba osób w poczekalni)-pamięć współdzielona:
  - wartość tę inkrementuje klient gdy siada w poczekalni   natomiast dekrementuje ją fryzjer gdy bierze klienta z poczekalni na fotel

  - dostęp do num_clients jest synchronizowany przy użyciu semafora binarnego - num_clients_mutex

- ch(fotele)-semafor:
  - odpowiada za liczbę wolnych foteli
  - jeśli żaden fotel nie jest wolny to fryzjer czeka na to aż jakiś się zwolni

- waiting_barbers_num(liczba oczekujących fryzjerow na wydanie reszty) - pamięć współdzielona:
  - za jej pomocą sprawdzamy czy wszyscy aktualnie działający fryzjerzy czekają na resztę
  - jeśli tak to ostatni oczekujący fryzjer zapisuje dług jaki salon fryzjerski ma w stosunku do obslugiwanego przez niego klienta
  - dzięki temu unikamy zakleszczenia
  - dług salonu wobec klienta jest zmniejszany przy jego następnym przybyciu
  - dostęp do tej pamięci chroniony jest semaforem binarnym waiting_barber_mutex

- waiting_barbers_queue_id(kolejka w oczekiwaniu na wydanie reszty) - semafor:
  - fryzjerzy którzy oczekują na wydanie reszty oczekują na tym semaforze 
    - gdy jakis fryzjer który na wydanie reszty nie oczekuje po w rzuceniu pieniędzy do kasy za usługę poinformuje
      jednego oczekującego fryzjera o nowej wartosci kasy poprzez podniesienie tego semafora

### INSTRUKCJA URUCHOMIENIA

 - Plik "make.sh" usuwa istniejące obiekty ipc, kompiluje kody i uruchamia kod programu "main".

 - Plik "main" umożliwia uruchomienie wielu klientów i fryzjerów wraz z ustawieniem początkowej wartości pieniędzy w kasie salonu.

 - W pliku "klient" oraz "fryzjer" należy ustawić te samą wartość dla NUM_CLIENTS, określa ona wielkość poczekalni.
