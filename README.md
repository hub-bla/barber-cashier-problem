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


### INSTRUKCJA URUCHOMIENIA

 - Plik "make.sh" usuwa istniejące obiekty ipc, kompiluje kody i uruchamia kod programu "main".

 - Plik "main" umożliwia uruchomienie wielu klientów i fryzjerów wraz z ustawieniem początkowej wartości pieniędzy w kasie salonu.

 - W pliku "klient" oraz "fryzjer" należy ustawić te samą wartość dla NUM_CLIENTS, określa ona wielkość poczekalni.
