# Core Speedtest

C programavimo kalba sukurta interneto greičio testavimo programa.

Programa gali nustatyti vartotojo vietovę, pasirinkti geriausią serverį pagal vietovę bei atlikti duomenų parsisiuntimo ir išsiuntimo greičio testus.

## Naudojamos bibliotekos

* **libcurl** – HTTP užklausoms ir duomenų siuntimo testams
* **cJSON** – JSON duomenų apdorojimui
* **getopt** – komandų eilutės parametrų valdymui

## Projekto struktūra

```text
Core/
├── src/
│   ├── main.c
│   ├── download.c
│   ├── upload.c
│   ├── server.c
│   └── geolocation.c
├── inc/
│   ├── download.h
│   ├── upload.h
│   ├── server.h
│   └── geolocation.h
├── data/
│   └── speedtest_server_list.json
├── build/
├── Makefile
└── README.md
```

## Reikalavimai

Reikalinga C kompiliavimo aplinka ir šios bibliotekos:

* GCC
* libcurl
* cJSON
* getopt

Programa buvo testuojama naudojant **MSYS2 UCRT64** aplinką.

## Diegimas

```bash
git clone https://github.com/Woxer2/Core.git
cd Core
```

## Kompiliavimas

Projekto kataloge paleiskite:

```bash
make
```

Norint išvalyti ankstesnio kompiliavimo rezultatus:

```bash
make clean
```

Po sėkmingo kompiliavimo programa sukuriama:

```text
build/main.exe
```

## Naudojimas

Pagal numatytuosius nustatymus galima atlikti visą automatizuotą testą:

```bash
./build/main.exe -a
```

Programa taip pat leidžia pasirinkti konkretų veiksmą.

### Vietovės nustatymas

```bash
./build/main.exe -l
```

Nustatoma vartotojo valstybė naudojant geolocation API.

### Geriausio serverio paieška

```bash
./build/main.exe -s
```

Programa nustato vartotojo valstybę, užkrauna serverių sąrašą ir parenka geriausią pasiekiamą serverį.

### Download testas

Automatiškai parenkant serverį:

```bash
./build/main.exe -d
```

Naudojant konkretų serverį:

```bash
./build/main.exe -d -H speedtest.litnet.lt:8080
```

### Upload testas

Automatiškai parenkant serverį:

```bash
./build/main.exe -u
```

Naudojant konkretų serverį:

```bash
./build/main.exe -u -H speedtest.litnet.lt:8080
```

### Konkrečios valstybės naudojimas

Valstybę galima nurodyti rankiniu būdu:

```bash
./build/main.exe -s -c Lithuania
```

Taip pat:

```bash
./build/main.exe -d -c Germany
```

arba:

```bash
./build/main.exe -u -c Germany
```

Jeigu `-c` nenurodytas, valstybė nustatoma automatiškai.

### Pagalba

```bash
./build/main.exe -h
```

Parodomas galimų veiksmų ir parametrų sąrašas.

## Komandų parametrai

| Parametras   | Aprašymas                        |
| ------------ | -------------------------------- |
| `-a`         | Atlieka visą automatizuotą testą |
| `-l`         | Nustato vartotojo vietovę        |
| `-s`         | Suranda geriausią serverį        |
| `-d`         | Atlieka download testą           |
| `-u`         | Atlieka upload testą             |
| `-h`         | Parodo pagalbą                   |
| `-H HOST`    | Naudoja konkretų serverį         |
| `-c COUNTRY` | Nurodo valstybę rankiniu būdu    |

Vienu metu galima pasirinkti tik vieną pagrindinį veiksmą.

`-H` galimas tik kartu su `-d` arba `-u`. `-c` galimas tik kartu su `-s`, `-d` arba `-u`. Panaudojus juos su kitu veiksmu (pvz. `-a -H ...`), programa grąžina klaidą ir baigia darbą su ne nuliniu (`non-zero`) exit kodu.

## Automatizuoto testo eiga

Naudojant:

```bash
./build/main.exe -a
```

atliekami šie veiksmai:

```text
Vietovės nustatymas
       ↓
Serverių sąrašo užkrovimas
       ↓
Geriausio serverio pasirinkimas
       ↓
Latency / jitter matavimas
       ↓
Download testas
       ↓
Upload testas
       ↓
Galutiniai rezultatai
```

## Rezultatai

Programa išveda:

* vartotojo valstybę;
* pasirinkto serverio tiekėją;
* serverio miestą;
* serverio adresą;
* latency;
* jitter;
* download greitį Mbps;
* upload greitį Mbps.

Pavyzdys:

```text
================================
        SPEEDTEST RESULTS
================================
Location : Lithuania
Server   : INIT
City     : Vilnius
Host     : speedtest-vno.init.lt:8080
--------------------------------
Latency  : 5.82 ms
Jitter   : 4.33 ms
Download : 752.31 Mbps
Upload   : 193.10 Mbps
================================
```

## Klaidų valdymas

Programa tikrina:

* geolocation API klaidas;
* serverių sąrašo užkrovimo klaidas;
* nepasiekiamus serverius;
* HTTP užklausų klaidas;
* neteisingus komandų eilutės parametrus;
* konfliktuojančius veiksmus;
* nesėkmingus download ir upload testus.

Neveikiantys serveriai serverio paieškos metu pažymimi kaip `FAILED` ir nėra pasirenkami kaip geriausias serveris.

## Testo trukmė

Download ir upload testams taikomas maksimalus **15 sekundžių** vykdymo laikas.

## Duomenų šaltinis

Serverių sąrašas saugomas faile:

```text
data/speedtest_server_list.json
```

Kai kurie sąraše esantys serveriai gali būti nebeaktyvūs, todėl programa juos praleidžia, jeigu nepavyksta prisijungti.

## Git

Repozitorija: [https://github.com/Woxer2/Core](https://github.com/Woxer2/Core)
