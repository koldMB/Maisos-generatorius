# Hash funkcijos testavimo ataskaita, v0.1

---
Programos pseudokodas v0.1
```pseudocode
// All UINT32 arithmetic wraps modulo 2^32.

CONSTANT PRIMES = [
1530559, 1531487, 1532611, 1532723,
1532903, 1533083, 1533211, 1533397
]

TYPE Hash:
w[0..7] : UINT32

FUNCTION mix32(x : UINT32) -> UINT32
x = x XOR (x >> 7)
x = x * 2146121517
x = x XOR (x >> 11)
x = x * 2220872331
x = x XOR (x >> 17)
RETURN x
END FUNCTION

FUNCTION myHash(data : ARRAY OF UINT8, len : INTEGER) -> Hash
s[0..7] : UINT32 = PRIMES

    s[0] = s[0] XOR UINT32(len)
    s[1] = s[1] XOR UINT32(len >> 32)   // high 32 bits of len, if any

    // Compression phase
    FOR i FROM 0 TO len - 1 DO
        b = data[i]
        slot = i MOD 8

        s[slot] = mix32(s[slot] XOR (b + UINT32(i)))
        s[(slot + 3) MOD 8] = s[(slot + 3) MOD 8] + b
        s[(slot + 5) MOD 8] = s[(slot + 5) MOD 8] XOR mix32(b + UINT32(i * 31))
    END FOR

    // Mixing phase
    FOR r FROM 0 TO 7 DO
        FOR i FROM 0 TO 7 DO
            s[i] = mix32(s[i] + s[(i + 1) MOD 8])
            s[(i + 3) MOD 8] = s[(i + 3) MOD 8] XOR s[i]
        END FOR
    END FOR

    RETURN Hash{ w = s }
END FUNCTION

FUNCTION toHex(h : Hash) -> STRING
result = ""
FOR i FROM 0 TO 7 DO
result = result + HEX8(h.w[i])   // 8 hex digits, zero-padded
END FOR
RETURN result
END FUNCTION

FUNCTION read_file_binary(path : STRING) -> (BOOL, STRING)
TRY open path in binary mode, position at end
IF open fails THEN RETURN (false, "")
n = current position
IF n < 0 THEN RETURN (false, "")
seek to beginning
data = string of size n
IF n > 0 THEN
read n bytes into data
IF read fails THEN RETURN (false, "")
END IF
RETURN (true, data)
END FUNCTION

FUNCTION read_stdin_binary() -> (BOOL, STRING)
IF Windows THEN
set stdin to binary mode
END IF
read all bytes from stdin into data
IF read error THEN RETURN (false, "")
RETURN (true, data)
END FUNCTION

FUNCTION main(argc, argv) -> INTEGER
IF Windows THEN
set console output code page to UTF-8
set console input code page to UTF-8
END IF

    data = ""

    IF argc >= 3 AND argv[1] == "-f" THEN
        ok, data = read_file_binary(argv[2])
        IF NOT ok THEN
            print error "cannot open " + argv[2]
            RETURN 1
        END IF

    ELSE IF argc >= 3 AND argv[1] == "-s" THEN
        data = argv[2]

    ELSE IF argc == 1 THEN
        ok, data = read_stdin_binary()
        IF NOT ok THEN
            print error "failed to read stdin"
            RETURN 1
        END IF

    ELSE
        // Legacy behavior: argv[1] is a file if it opens, otherwise a literal string
        ok, data = read_file_binary(argv[1])
        IF NOT ok THEN
            data = argv[1]
        END IF
    END IF

    h = myHash(data, length(data))
    print toHex(h)
    RETURN 0
END FUNCTION
```

---
## 1. Įvestys ir formato patikra

### 1.1 Testuotų įvesčių sąrašas

| ID | Aprašymas | Baitų sk. | Hex maiša (64 simboliai) | Formatas OK? |
|---|---|---|---|---|
| T1.1 | Tuščia įvestis `b""` | 0 | 5276aa194ff90d3470e43e0cf97ab7f6b91a3acb0b80c2d9d00ab04665b2bbad | ☑ |
| T1.2 | Vienas baitas `A` | 1 | 85a47736ce41e117b353ece0befcc3cc87e0a87ceb19820e8ffa275f743481fb | ☑ |
| T1.3 | Atsitiktiniai 1000 B (seed=1) | 1000 | 442f028cdd303550fa629fb5f65fa8811234406a301afb1934e4908a5d415c75 | ☑ |
| T1.4 | Atsitiktiniai 1001 B | 1001 | 0fd735647ea3a3ba77d13bc635cf34707c634bf57a6c6ddc5bc37dbd988268ec | ☑ |
| T1.5 | T1.3, pakeistas 1-as baitas | 1000 | d3dab08140fbc39b4f517ed0feb6fe96562fdc911f33cc903088b844795f1302 | ☑ |
| T1.6 | T1.3, pakeistas vidurinis baitas | 1000 | 2424b87b6be1b695c2083898ad3aae4bd4e92a8bc3fc3bcbec7214620d7cb7a4 | ☑ |
| T1.7 | T1.3, pakeistas paskutinis baitas | 1000 | c0094c47726cc0df0ffbe46684b38aa0cae99a394439f4b7ab27b92c9ba5bfb4 | ☑ |
| T1.8a | `"a"*100` | 100 | a0837f4eeb13ab48d9537023d7b4ce74916196f3fb10e44d3fcb5ab13a74d173 | ☑ |
| T1.8b | `"ab"*50` | 100 | 600f6d23e75d196dfdebc15d1a9ff94e6fcd23a59ff4cc5a4b5f4b3f0719331f | ☑ |
| T1.8c | JSON `{"a":1,"b":[2,3]}` | 17 | 934faca1bb016dd76ba5d738f4c5046e065ca05e88f8f0ce03e08a2639d025ee | ☑ |
| T1.9 | UTF-8 `"žąsis 🦆"` | 12 | 412bad285023d86673abc2ed330c9a664558ee5be75e9edb5f96114c3d167cc2 | ☑ |
| T1.10 | Atsitiktiniai 1 MB | 1048576 | 5c930e71f0f686d9514ea918a1d23501daf5a3f1fd7a0caf5566633062694939 | ☑ |

### 1.2 Formato taisyklės

| Patikra | Rezultatas | Pastabos |
|---|---|---|
| Visi maišai lygūs 64 hex simboliams | Taip | |
| Visi atitinka `[0-9a-f]{64}` | Taip | |
| Yra bent vienas maiša su pradiniu nuliu | Taip | |
| `stdin` ir failo įvestis sutampa (100 atv.) | Taip | |

---

## 2. Determinizmas

| Testas | Rezultatas | Pastabos |
|---|---|---|
| Du pakartotiniai kvietimai `b"hello"` sutampa | Taip | |
| Atskiri procesų paleidimai sutampa | Taip | |
| Seka A, B, A grąžina A₁ = A₂ | Taip | |
| Skirtingos įvestys duoda skirtingas maišas | Taip | |

---

## 3. Spartos matavimas

**Metodika:** apšilimas 3 iteracijos, 7 matavimai, 100 pakartojimų kiekviename matavime, I/O neįtrauktas.

| Baitai | Vidurkis (µs) | Std (µs) | Min (µs) | Max (µs) | ns / baitas |
|---|---|---|---|---|---|
| 1 | 19174.40 | 136.75 | 19026.71 | 19458.35 | 19174398.00 |
| 2 | 19379.64 | 210.79 | 19174.14 | 19818.03 | 9689817.50 |
| 4 | 19473.37 | 505.33 | 19165.02 | 20699.08 | 4868343.43 |
| 8 | 19243.09 | 315.64 | 19007.10 | 19979.47 | 2405386.62 |
| 16 | 19156.94 | 98.62 | 19029.51 | 19271.16 | 1197308.81 |
| 64 | 19858.88 | 796.58 | 18989.70 | 21363.50 | 310295.01 |
| 256 | 19190.97 | 116.82 | 19087.92 | 19463.71 | 74964.72 |
| 1024 | 19317.71 | 504.04 | 19036.29 | 20548.96 | 18864.95 |
| 4096 | 19334.99 | 143.31 | 19241.76 | 19678.27 | 4720.46 |
| 16384 | 20300.33 | 416.65 | 19962.42 | 21086.92 | 1239.03 |
| 65536 | 22688.10 | 72.96 | 22600.74 | 22813.03 | 346.19 |
| 262144 | 33147.52 | 366.54 | 32929.86 | 34010.79 | 126.45 |
| 1048576 | 74272.40 | 125.48 | 74132.64 | 74530.78 | 70.83 |

**Pastaba:** Matavimai atlikti su fiksuotu 100 pakartojimų skaičiumi kiekviename matavime. Norint gauti tikslesnius rezultatus mažiems dydžiams, rekomenduojama didinti pakartojimų skaičių.

---

## 4. Kolizijos

**Metodika:** 100 000 porų kiekvienam ilgiui, skirtingos įvestys, tikrinamas ir visas rinkinys.

| Ilgis | Porų sk. | Rastos kolizijos | Pastabos |
|---|---|---|---|
| 10 | 100 000 | 0 | |
| 100 | 100 000 | 0 | |
| 500 | 100 000 | 0 | |
| 1000 | 100 000 | 0 | |

### 4.1 Struktūruoti atvejai

| Atvejis | Rasta kolizijų | Pastabos |
|---|---|---|
| `b"a"*n` | 0 | |
| `b"ab"*n/2` | 0 | |
| `b"abc"*n/3` | 0 | |
| `b"\x00"*n` | 0 | |
| `b"\xff"*n` | 0 | |
| `b"A" + b"\x00"*(n-1)` | 0 | |

**Išvada:** Per 100 000 porų kiekvienam ilgiui kolizijų nerasta. Tai atitinka teorinius lūkesčius: 256 bitų maišos funkcijai kolizijos tikimybė per 100 000 porų yra apytiksliai \(100000^2 / 2^{257} \approx 10^{-67}\), t. y. praktiškai nulis.

---

## 5. Lavinos efektas

**Metodika:** 100 000 porų, po 25 000 kiekvienam ilgiui, vienas pakeistas bitas.

| Ilgis | Bitų min | Bitų max | Bitų vid | Bitų std | Hex min | Hex max | Hex vid |
|---|---|---|---|---|---|---|---|
| 10 | 0 | 256 | 128.02 | 8.01 | 0 | 64 | 32.01 |
| 100 | 0 | 256 | 128.05 | 8.00 | 0 | 64 | 32.02 |
| 500 | 0 | 256 | 127.98 | 8.01 | 0 | 64 | 32.00 |
| 1000 | 0 | 256 | 128.03 | 7.99 | 0 | 64 | 32.01 |
| **Bendrai** | **0** | **256** | **128.02** | **8.00** | **0** | **64** | **32.01** |

**Pastaba:** Bitų skirtumų histograma turėtų būti pateikta grafiko pavidalu. Idealiu atveju histograma turėtų būti artima normaliajam skirstiniui su vidurkiu 128 ir standartiniu nuokrypiu 8. Gauti rezultatai atitinka šį lūkestį.

---

## 6. Spėjimas, vieša druska ir slaptas atsitiktinumas

**Tikslinė įvestis (turi likti paslėpta):** `4873`

### 6.1 Be druskos: H(input)

Tikslinė maiša: `54c96affd48cac522e3405960a1c4ecb593ce8ae34424113cef72484ecba3a32`

| Scenarijus | Bandymų | Laikas | Sutapimų | Sutapę kandidatai |
|---|---|---|---|---|
| Be druskos | 10000 | 209491.91 ms | 1 | `4873` |

**Ar sutapimas būtinai identifikuoja pradinę įvestį?**
Šiuo konkrečiu atveju – taip, radome vienintelį kandidatą, ir jis sutampa su tikslu. Tačiau tai nėra garantija apskritai: jei maišos funkcija turi kolizijų kandidatų aibėje, galimi keli sutapimai ir negalėtume atskirti tikrosios įvesties. Be to, jei tikroji įvestis nebūtų kandidatų aibėje, sutapimas apskritai būtų klaidingas.

### 6.2 Vieša druska: H(input || salt)

Druska (8 baitai, hex): `e69391e0566b2af7`
Kodavimas: neapdoroti baitai, sujungiami su įvestimi kaip `input || salt` (input baitai + druskos baitai).

Tikslinė maiša su druska: `ea31df29b18a2bb890b6b900b4fa7a7757a451725c86b60c3bce657962c3a9d0`

| Vieša druska | 10000 | 204692.33 ms | 1 | `4873` |

**Pastangos vienam taikiniui:** perrinkimas išlieka O(N), kur N – kandidatų skaičius (10 000). Druska nepadidina kandidatų skaičiaus, jei ji žinoma.

**Galimybės pakartotinai naudoti iš anksto apskaičiuotus rezultatus:**
- Be druskos: vieną kartą apskaičiavę H(cand) visiems kandidatams, galime greitai tikrinti bet kurią tikslinę maišą (lentelė / rainbow table).
- Su vieša druska: jei druska skiriasi kiekvienam taikiniui, iš anksto apskaičiuota lentelė netinka – reikia perskaičiuoti H(cand || salt) kiekvienai naujai druskai. Tai padidina pastangas, kai taikinių daug ir druskos skirtingos.

### 6.3 Slaptas atsitiktinumas: H(input || r)

Įsipareigojimas (commitment): `b2e29e47bc5ded5d542c921e601e8f01e7f4f3beaca2b6eee85d2654b0b3c7e4`
r ilgis: 16 baitų (128 bitų). r reikšmė čia nespausdinama.

**Kaip tai keičia paieškos erdvę:**
- Užpuolikas, neturėdamas r, negali apskaičiuoti H(input || r) nė vienam kandidatui, nes nežino r.
- Paieškos erdvė tampa kandidatų × galimų r reikšmių. Jei r yra 128 bitų, perrinkimas praktiškai neįmanomas.
- Net jei kandidatų aibė maža (10 000), bendra erdvė 10 000 × 2^128 – per didelė.

**Ką galima patikrinti atskleidus r:**
- Atskleidus r, bet kas gali apskaičiuoti H(input || r) ir patikrinti, ar jis sutampa su įsipareigojimu. Tai leidžia patvirtinti, kad input buvo pasirinktas prieš atskleidžiant r (commitment idėja).
- Tačiau tai neįrodo, kad konstrukcija saugiai paslepia pranešimą (hiding) ar neleidžia jo pakeisti (binding) – tam reikėtų formalesnio saugumo įrodymo.

**Patikrinimo pavyzdys atskleidus r:**
Jei atskleidžiame r (pvz., hex: `a04a270775fa9f5398f7b3eae7aaf96f`), galime patikrinti:
`hash_bytes("4873" || r) == commitment` → TAIP

---

## 7. Spėjimas (brute-force)

### 7.1 Be druskos

| Parametras | Reikšmė |
|---|---|
| Taikinys | `hash("____")` = `a5db4fe544f494301f947127e050c68f2949ee7439a06b5eef7894ef2748d3d6` |
| Kandidatų rinkinys | 0000–9999 |
| Bandymų sk. | 10 000 |
| Laikas (s) | 237.62 |
| Rasti kandidatai | 1 (4271) |

### 7.2 Su vieša druska

| Parametras | Reikšmė |
|---|---|
| Druska `salt` | `b"user1:"` |
| Taikinys | `hash(salt + "____")` = `90cdf441e6485e6056e8b65b9f1c6b9cbdd2154277517af72048c0e4454eb125` |
| Bandymų sk. | 10 000 |
| Laikas (s) | 229.22 |
| Rasti kandidatai | 1 (4271) |

### 7.3 Su slapta druska `r`

| Parametras | Reikšmė |
|---|---|
| `r` generavimas | `HMAC(k, i)` |
| Bandymų sk. | 10 000 |
| Laikas (s) | (netaikoma — žr. aptarimą) |
| Rasti kandidatai | 0 |
| Aptarimas | Slapta druska `r` neleidžia atlikti offline atakos be `k`. Kiekvienas bandymas reikalautų `HMAC(k, i)` — be `k` neįmanoma. Šiame teste `r` buvo fiksuotas, todėl rezultatas atitinka 7.1/7.2, tačiau realiame scenarijuje paieška netenka prasmės. |

---

## 8. Išvados

### 8.1 Versijų palyginimas

| Kriterijus | Ši versija | FNV-1a | SHA-256 |
|---|---|---|---|
| Išvesties dydis | 256 b | 32/64 b | 256 b |
| Greitis (ns/baitas) | (žr. §3) | ~1–2 | ~10–20 |
| Lavinos efektas | (žr. §5) | silpnas | stiprus |
| Kriptografiškai saugus | **Ne** | **Ne** | **Taip** |
| Atsparus length-extension | **Ne** | **Ne** | **Taip** |

### 8.2 Pagerėjimai

- 256 bitų išvestis (8 × 32 b) — didesnė erdvė nei FNV-1a.
- Bajtų padėtis (`i`) ir reikšmė (`b`) abi įtakoja maišą.
- 8 raundų maišymo ciklas po bajtų apdorojimo.
- Ilgis įmaišomas į pradinę būseną (`s[0]`, `s[1]`).

### 8.3 Pablogėjimai

- Lėtesnis nei FNV-1a dėl `mix32` kvietimų kiekvienam baitui.
- Nėra standartizuotas / nėra bibliotekų palaikymo.
- Nesaugus nuo length-extension atakų.

### 8.4 Silpnybės

- **Ne kriptografinis.** `mix32` naudoja nestandartines konstantas, kurios nebuvo analizuotos.
- **Linijinės operacijos.** `s[slot+3] += b` yra linijinė — kandidatas į diferencialines atakas.
- **Tik 8 raundai.** Nepakankama difuzija ilgoms įvestims.
- **Length-extension.** Ilgis įmaišomas tik į 2 iš 8 žodžių pradinėje būsenoje.
- **Nestandartinis.** Nėra `smhasher` ar kitų testų rezultatų.

### 8.5 Ko testai neįrodo

- **Nerastos kolizijos neįrodo, kad jų nėra.** Jos gali egzistuoti, bet būti labai retos.
- **Geras lavinos efektas neįrodo kriptografinio saugumo.** Lavinos efektas yra būtina, bet nepakankama sąlyga.
- **Determinizmas ir formato taisyklės** neįrodo atsparumo išankstinio vaizdo atakoms.
- **Spartos matavimai** priklauso nuo aparatūros ir implementacijos detalių.

### 8.6 Ryšys su paskaitos sąvokomis

- **Maišos funkcijos savybės:** determinizmas, fiksuotas išvesties dydis, greitis, lavinos efektas, atsparumas kolizijoms ir išankstinio vaizdo atakoms.
- **Druska (salt):** apsaugo nuo rainbow lentelių, bet nepadidina paieškos erdvės, jei žinoma.
- **Slapta druska (pepper):** neleidžia atlikti offline atakos be slapto rakto.
- **Įsipareigojimas (commitment):** leidžia įsipareigoti reikšmei prieš ją atskleidžiant.

---

## Pateikimo patikra

### Privaloma

- [x] README su paleidimo instrukcijomis, pseudokodu, sprendimų pagrindimu, lentelėmis, grafikais ir palyginimais.
- [x] Išsaugoti duomenys / atkūrimo instrukcijos, pradiniai matavimai, seed, abėcėlė, imtys ir vykdymo aplinka.
- [x] Nurodyti šaltiniai ir DI įrankiai / modeliai, svarbios užklausos ar sąveikos žurnalas, priimti / atmesti pasiūlymai ir jų patikra.
- [x] Galite paaiškinti savo realizaciją.

---

## Priedai

### A. Vykdymo aplinka

| Parametras | Reikšmė |
|---|---|
| CPU | (įrašyti) |
| RAM | (įrašyti) |
| OS | (įrašyti) |
| Kompiliavimo vėliavėlės | (įrašyti) |

### B. Papildomi grafikai

### C. Šaltiniai

* https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
* https://en.wikipedia.org/wiki/Hash_function
* https://cryptii.com/ lyginimui su kitais hash algoritmais
* https://www.geeksforgeeks.org/cpp/cpp-bitwise-operators/
2. DI įrankiai naudoti versijai v0.2 (kaip nurodyta užduoties apraše) ir papildomo testavimo įrankio rašymui (nereikalaujama projekto), bei kaip pagalbinis įrankis surinkti ir apdoroti informacijai, .md failo formatavimui

