# Dokumentacija — Battery Monitoring System

## Pregled sustava

Sustav za praćenje baterija sastoji se od tri komponente koje zajedno čine end-to-end rješenje:

```
ESP32 → internet (hotspot) → ngrok tunel → Node.js server → PostgreSQL baza
```

ESP32 mikrokontroler mjeri napon, temperaturu i vlagu te šalje podatke na server. Server sprema podatke u bazu. Frontend dashboard čita podatke iz baze i prikazuje ih korisniku.

---

## Komponente

### 1. ESP32 mikrokontroler

ESP32 je zadužen za:
- Spajanje na WiFi (mobilni hotspot)
- Očitavanje temperature i vlage putem AHT20 senzora (I2C, pinovi 21 i 22)
- Mjerenje napona putem naponskog djelitelja (pin 34, R1=33kΩ, R2=10kΩ)
- Slanje podataka na server putem HTTP POST zahtjeva

Mjerenje se pokreće pritiskom na touch senzor (pin 4). Nakon pritiska ESP32 izvršava 8 mjerenja u razmaku od 2 sekunde te za svako mjerenje šalje POST zahtjev na server.

**Važno:** ESP32 šalje `current: 0` jer nema senzor struje u trenutnoj verziji projekta.

### 2. Node.js / Express backend (sensor-api)

Server prima podatke od ESP32 i pruža API za frontend.

| Metoda | Putanja | Opis |
|--------|---------|------|
| GET | `/api/readings` | Dohvati zadnjih 100 očitavanja |
| GET | `/api/devices` | Dohvati sve registrirane uređaje |
| POST | `/api/readings` | ESP32 šalje novo očitavanje |
| POST | `/api/auth/login` | Prijava korisnika, vraća JWT token |

Server automatski registrira novi uređaj ako primi nepoznati `api_key`. Ime uređaja generira se kao `Device-<prvih 6 znakova api_key-a>`.

**Pokretanje servera:**
```bash
npm install
node index.js
# Server sluša na http://0.0.0.0:3000
```

Server mora slušati na `0.0.0.0` (a ne `localhost`) da bi bio dostupan s vanjske mreže kroz ngrok tunel.

### 3. PostgreSQL baza podataka

Baza sadrži tri tablice:

**devices** — registrirani uređaji
- `id`, `name`, `serial_number`, `api_key_hash`, `is_active`, `created_at`

**battery_readings** — očitavanja senzora
- `id`, `device_id` (FK), `voltage`, `current`, `temperature`, `recorded_at`, `created_at`

**users** — korisnici frontenda
- `id`, `email`, `password_hash`, `full_name`

---

## Problem s mrežom i rješenje (ngrok)

### Zašto direktna konekcija ne radi

Kada su ESP32 i laptop spojeni na isti mobilni hotspot (iPhone ili Android), između njih postoji **client isolation** — uređaji na hotspotu ne mogu međusobno komunicirati. ESP32 pokušava dosegnuti laptop na lokalnoj IP adresi (`172.20.10.10:3000`) ali dobiva grešku `connection refused` (HTTP kod `-1`).

Ovo je problem svakog mobilnog hotspota i ne može se zaobići s konfiguracije na strani laptopa.

### Rješenje: ngrok tunel

ngrok stvara javni URL na internetu koji tunelira promet do lokalnog servera na laptopu. Umjesto da se ESP32 spaja direktno na laptop, spaja se na ngrok server koji dalje prosljeđuje zahtjev.

```
ESP32 → internet → ngrok.io → laptop:3000
```

ESP32 ne treba biti na istoj mreži kao laptop — dovoljno je da ima pristup internetu.

---

## Postupak pokretanja sustava

### Korak 1 — Pokretanje servera

Otvori prvi terminal i pokreni:
```bash
node index.js
```

Server mora ostati aktivan cijelo vrijeme.

### Korak 2 — Pokretanje ngrok tunela

Otvori **drugi terminal kao Administrator** (Windows + X → Terminal (Admin)) i pokreni:
```bash
ngrok http 3000
```

ngrok će ispisati forwarding URL koji izgleda ovako:
```
Forwarding  https://tvoja-adresa.ngrok-free.dev -> http://localhost:3000
```

**Važno:** Besplatni ngrok račun generira **novi URL pri svakom pokretanju**. URL se mora ažurirati u ESP32 kodu pri svakom novom pokretanju ngroka.

### Korak 3 — Ažuriranje URL-a u ESP32 kodu

Kopirati novi ngrok URL i upisati ga u `SERVER_URL` u datoteci `secrets.h`:
```cpp
#define SERVER_URL "https://NOVI-URL.ngrok-free.dev/api/readings"
```


### Korak 4 — Flash ESP32

Uploadanje koda na ESP32 kroz Arduino IDE 

### Korak 5 — Testiranje

Pritisak touch senzora na ESP32. U Serial Monitoru (115200 baud) trebao bi se pojaviti:
```
Touch senzor pritisnut! Počinjem mjerenje...
Mjerenje 1/8
Temperatura: 24.50 °C
Vlaga: 45.20 %
Napon: 3.85 V
Šaljem: {"api_key":"ESP32-UREDAJ-001","temperature":24.5,"voltage":3.85,"current":0}
Podaci uspješno poslani!
```

---

## Autentikacija ngroka

ngrok zahtijeva besplatan korisnički račun i authtoken.

1. Registracija na [dashboard.ngrok.com](https://dashboard.ngrok.com)
2. Kopiranje authtokena s dashboarda
3. Dodavanje tokena u konfiguraciju:
```bash
ngrok config add-authtoken TVOJ_TOKEN
```

Konfiguracija se sprema u:
```
C:\Users\IME\AppData\Local\ngrok\ngrok.yml
```

---

### Gašenje Node.js servera
Ako je potrebno ugasiti server koji već radi:
```bash
# Pronađi PID procesa
netstat -ano | findstr :3000

# Ugasi po PID-u
taskkill /PID BROJ /F

# Ili ugasi sve Node procese
taskkill /IM node.exe /F
```

---
