# Hash funkcijos testavimo ataskaita

**Data:** ___________

---

## 1. Įvestys ir formato patikra

### 1.1 Testuotų įvesčių sąrašas

| ID | Aprašymas | Baitų sk. | Hex maiša (64 simboliai) | Formatas OK? |
|---|---|---|---|---|
| T1.1 | Tuščia įvestis `b""` | 0 | | ☐ |
| T1.2 | Vienas nulinis baitas `\x00` | 1 | | ☐ |
| T1.3 | Vienas baitas `A` | 1 | | ☐ |
| T1.4 | Atsitiktiniai 1000 B (seed=1) | 1000 | | ☐ |
| T1.5 | Atsitiktiniai 1001 B | 1001 | | ☐ |
| T1.6 | T1.4, pakeistas 1-as baitas | 1000 | | ☐ |
| T1.7 | T1.4, pakeistas vidurinis baitas | 1000 | | ☐ |
| T1.8 | T1.4, pakeistas paskutinis baitas | 1000 | | ☐ |
| T1.9a | `"a"*100` | 100 | | ☐ |
| T1.9b | `"ab"*50` | 100 | | ☐ |
| T1.9c | JSON `{"a":1,"b":[2,3]}` | 17 | | ☐ |
| T1.10 | UTF-8 `"žąsis 🦆"` | | | ☐ |
| T1.11 | Atsitiktiniai 1 MB | 1048576 | | ☐ |

### 1.2 Formato taisyklės

| Patikra | Rezultatas | Pastabos |
|---|---|---|
| Visi maišai lygūs 64 hex simboliams | ☐ Taip ☐ Ne | |
| Visi atitinka `[0-9a-f]{64}` | ☐ Taip ☐ Ne | |
| Yra bent vienas maišas su pradiniu nuliu | ☐ Taip ☐ Ne | |
| `stdin` ir failo įvestis sutampa (100 atv.) | ☐ Taip ☐ Ne | |

---

## 2. Determinizmas

| Testas | Rezultatas | Pastabos |
|---|---|---|
| Du pakartotiniai kvietimai `b"hello"` sutampa | ☐ Taip ☐ Ne | |
| Atskiri procesų paleidimai sutampa | ☐ Taip ☐ Ne | |
| Seka A, B, A grąžina A₁ = A₂ | ☐ Taip ☐ Ne | |
| Skirtingos įvestys duoda skirtingas maišas | ☐ Taip ☐ Ne | |

---

## 3. Spartos matavimas

**Metodika:** apšilimas 3 iteracijos, 7 matavimai, 100 pakartojimų kiekviename matavime, I/O neįtrauktas.

| Baitai | Vidurkis (µs) | Std (µs) | Min (µs) | Max (µs) | ns / baitas |
|---|---|---|---|---|---|
| 1 | | | | | |
| 2 | | | | | |
| 4 | | | | | |
| 8 | | | | | |
| 16 | | | | | |
| 64 | | | | | |
| 256 | | | | | |
| 1024 | | | | | |
| 4096 | | | | | |
| 16384 | | | | | |
| 65536 | | | | | |
| 262144 | | | | | |
| 1048576 | | | | | |

**Grafikas:** `bench.png` (x = baitai log, y = ns / baitas) ☐ Paruoštas

---

## 4. Kolizijos

**Metodika:** 100 000 porų kiekvienam ilgiui, skirtingos įvestys, tikrinamas ir visas rinkinys.

| Ilgis | Porų sk. | Rastos kolizijos | Pastabos |
|---|---|---|---|
| 10 | 100 000 | | |
| 100 | 100 000 | | |
| 500 | 100 000 | | |
| 1000 | 100 000 | | |

### 4.1 Struktūruoti atvejai

| Atvejis | Rasta kolizijų | Pastabos |
|---|---|---|
| `b"a"*n` | | |
| `b"ab"*n/2` | | |
| `b"abc"*n/3` | | |
| `b"\x00"*n` | | |
| `b"\xff"*n` | | |
| `b"A" + b"\x00"*(n-1)` | | |

---

## 5. Lavinos efektas

**Metodika:** 100 000 porų, po 25 000 kiekvienam ilgiui, vienas pakeistas bitas.

| Ilgis | Bitų min | Bitų max | Bitų vid | Bitų std | Hex min | Hex max | Hex vid |
|---|---|---|---|---|---|---|---|
| 10 | | | | | | | |
| 100 | | | | | | | |
| 500 | | | | | | | |
| 1000 | | | | | | | |
| **Bendrai** | | | | | | | |

**Teorinis vidurkis:** 128 / 256 bitų (50 %), 60 / 64 hex (93,75 %).

**Histograma:** `avalanche_hist.png` ☐ Paruošta

---

## 6. Spėjimas (brute-force)

### 6.1 Be druskos

| Parametras | Reikšmė |
|---|---|
| Taikinys | `hash("____")` |
| Kandidatų rinkinys | 0000–9999 |
| Bandymų sk. | 10 000 |
| Laikas (s) | |
| Rasti kandidatai | |

### 6.2 Su vieša druska

| Parametras | Reikšmė |
|---|---|
| Druska `salt` | `b"user1:"` |
| Taikinys | `hash(salt + "____")` |
| Bandymų sk. | 10 000 |
| Laikas (s) | |
| Rasti kandidatai | |

### 6.3 Su slapta druska `r`

| Parametras | Reikšmė |
|---|---|
| `r` generavimas | `HMAC(k, i)` |
| Bandymų sk. | 10 000 |
| Laikas (s) | |
| Rasti kandidatai | |
| Aptarimas | |

---

## 7. Išvados

### 7.1 Versijų palyginimas

| Kriterijus | Ši versija | FNV-1a | SHA-256 |
|---|---|---|---|
| Išvesties dydis | 256 b | 32/64 b | 256 b |
| Greitis (ns/baitas) | | | |
| Lavinos efektas | | | |
| Kriptografiškai saugus | ☐ Taip ☐ Ne | ☐ Taip ☐ Ne | ☐ Taip ☐ Ne |
| Atsparus length-extension | ☐ Taip ☐ Ne | ☐ Taip ☐ Ne | ☐ Taip ☐ Ne |

### 7.2 Pagerėjimai

- _______________________________________________
- _______________________________________________

### 7.3 Pablogėjimai

- _______________________________________________
- _______________________________________________

### 7.4 Silpnybės

- _______________________________________________
- _______________________________________________
- _______________________________________________

### 7.5 Ko testai neįrodo

- _______________________________________________
- _______________________________________________
- _______________________________________________

### 7.6 Ryšys su paskaitos sąvokomis

- _______________________________________________
- _______________________________________________
- _______________________________________________

---
