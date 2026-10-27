## Projekt

Das Projekt besteht darin, ein einstellbares Cache-Modul in System C zu entwerfen, welches Lese- und Schreibzugriffe unter Verwendung unterschiedlicher Mapping-Strategien (direct-mapped oder fully associative) durchführt. Das Cache soll an ein Hauptspeicher angeschlossen sein und die Adressen, welche gelesen bzw. geschrieben werden sollen als Signal bekommen. Hierbei gilt es, bis zu drei Cache-Level (L1, L2, L3) zu unterstützen und Cache Misses bzw. Hits entsprechend zu erkennen und zu behandeln. Der Cache soll außerdem inklusiv sein. Zudem sollen über ein Rahmenprogramm in C Funktionen zur Simulation und Evaluation implementiert werden.

## Literaturrecherche

Die Literaturrecherche ergab, dass moderne CPUs typischerweise über 2 bis 3 Cache-Ebenen verfügen. Der L1-Cache ist mit 16 bis 64 KiB relativ klein, aber sehr schnell (Latenz von etwa 1–3 Taktzyklen), während L2-Caches bis zu 512 KiB umfassen und Latenzen von etwa 4–12 Zyklen aufweisen. L3-Caches kann mehrere MiB groß sein, aber sind jedoch deutlich langsamer (Latenz 20–40 Zyklen), was sie nur als letzte Zwischenspeicherstufe vor dem Hauptspeicher sinnvoll macht.

Bezüglich der Mapping-Strategien zeigt sich, dass direct-mapped Caches zwar eine einfache Adressierung erlauben und hardwareseitig effizient sind, jedoch häufiger zu Konfliktmisses führen. Diese können die Allgemeine Performance des Caches stark einschränken. Fully-associative-Caches bieten mehr Flexibilität beim Ablegen von Daten, erfordern jedoch komplexere Steuerlogik und längere Zugriffszeiten.

Als Replacement-Verfahren haben wir in der Recherche vor allem LRU, LFU und FIFO als die bekanntesten Strategien gefunden. LRU funktioniert am besten bei typischen sequentiellen Zugriffen oder Schleifen, ist aber auch mit höherem Verwaltungsaufwand bei grösseren Caches verbunden. LFU ist ebenso eine relativ effiziente und weitbenutzte Strategie, leider kann es mit LFU zu cache Pollution kommen. In unserem Projekt wurde jedoch eine einfache FIFO-Strategie verwendet, um eine simple und einfach skalierbare Replacement-Methode darzustellen. Diese Strategie ist aber nicht perfekt, da sie Zugriffsmuster ignoriert und dem entsprechend immer eine konstante (aber nicht effiziente) Laufzeit hat. 

Um den Cache auf einer praktischen Aufwendung testen zu können haben wir ein Test zu einem speicherintensiven Matrixmultiplikation-Algorithmus geschrieben. Diese naive Implementierung benötigt O(n^2) Speicherplatz, da die resultierende Matrix aus n-Zeilen und n-Spalten besteht. Für jeden Eintrag der neuen Matrix müssen die Faktoren vom Speicher gelesen werden und dann nach deren Addition als Einträge zurück in den Speicher geschrieben werden. Ein speicherintensiver Algorithmus wie dieser erlaubt es, die Effizienzgewinne durch den Einsatz eines Caches zu beobachten und zu analysieren.

## Methodik und Messumgebung

Das Cache-Modul ist mit dem Layer-Modul als Untermodul implementiert, welches für jede Layer einmal instanziiert wird. Jede Cache-Layer ist als 2D-Vektor modelliert (Vektor von Vektor <uint8_t>). Alle Layers werden in entsprechender Reihenfolge durch ports verbunden und geben so untereinander enable-Signale weiter. Ein Cachezugriff wird demnach zuerst an die erste Layer gegeben und diese gibt ihn nur an die nächste Layer weiter wenn sie das Datum nicht bei sich findet, sollte auch die Letzte Layer nichts finden, wird der Request an das Main Memory Modul (auch Untermodul von Cache) weiter geleitet. Sobald ein Hit auftritt oder Main-Memory einen Request abgearbeitet hat, wird die Aktuelle Cachezeile von der höchsten Layer in der die Cachezeile vollständig liegt an die nächsthöhere gegeben (Promoted) bis der höchste Layer erreicht ist. Die Weitergabe von Requests wird durch enable-Signale und sc_events gelöst.

Das dazugehörige Programm in C wurde mit getopt_long erweitert, um Kommandozeilen Argumente wie die Anzahl der Zyklen, Cachegrößen, Latenzen, Pfade von Eingabe- oder auch Trace-Dateien zu verarbeiten und in der parsing.c Datei zu parsen. Im Terminal muss dabei noch der Pfad zu einer CSV Datei angegeben werden, durch welche der eigentliche Cache mithilfe der Request-Struct simuliert wird. Die CSV_reader.c liest dabei die CSV Datei und versucht zugleich die einzelnen Zeilen in Requests zu parsen als auch Fehler in der Datei zu finden.

Die Messungen wurden durch ein Result-Struct zusammengefasst, dieses enthielt die Anzahl der Hits, Misses und die dazugehörige Gesamtanzahl der Zyklen. Des Weiteren gibt es die Möglichkeit beim Aufruf des Programms ein Tracefile wichtiger Signale zu erstellen.

## Ergebnisse

Der implementierte Cache verhält sich in der Simulation erwartungsgemäß: Bei sequentiellen Zugriffen auf benachbarte Adressen treten nach dem erstmaligen Cache-Miss nur Cache-Hits auf, insbesondere wenn mit größeren Cachezeilen gearbeitet wurde. Bei Zugriﬀen auf weit auseinanderliegende Adressen war die Miss-Rate hoch, insbesondere wenn wenige Cachezeilen und ein direct-mapped Cache verwendet werden.

Dadurch, dass eine Cachezeile aus der Main-Memory nur in 4 Byte-Chunks gelesen werden kann erhöht sich die Simulationsdauer, um ein Vielfaches der Memory-Latency, was bei kleinen Datenmengen und großen Cachezeilen sehr ineffizient ist.

## Individuelle Leistungen

### *Rodrigo Martin Garcia*

Ich habe zusammen mit Miron den SystemC Teil implementiert. Wir haben gleichzeitig sowohl am Cache und Layer Modul gearbeitet, wobei ich überwiegend am Layer Modul gearbeitet habe.

### *Mahsar Eren Önel*

Ich habe das Rahmenprogramm und die Makefile geschrieben. Dazu gehört der CSV_reader, das parsing, der Number_converter und die main.c.

### *Miron Förster*

Ich habe zusammen mit Rodrigo den SystemC Teil implementiert. Hauptsächlich die Integrierung mit dem Rahmenprogramm und den overall Control-Flow.

https://dev.to/larapulse/cpu-cache-basics-57ej#:~:text=L1%20Cache:%20Typically%20ranges%20from,your%20code%20for%20specific%20hardware

https://www.geeksforgeeks.org/computer-organization-architecture/difference-between-direct-mapping-associative-mapping-set-associative-mapping/

https://github.com/91varunsharma/Multilevel-Cache-Simulator/blob/master/cachesimulator.cpp

https://dev.to/satrobit/cache-replacement-algorithms-how-to-efficiently-manage-the-cache-storage-2ne1

https://medium.com/%40Styp/why-loops-do-matter-a-story-of-matrix-multiplication-cache-access-patterns-and-java-859111845c25

# Nutzung

## 1. Programm kompilieren

```bash
make project
```

## 2. Programm starten

Nach erfolgreicher Kompilierung kann der Cache-Simulator über die Kommandozeile gestartet werden.

**Linux/macOS:**

```bash
./project --cycles 1000 requests.csv
```

**Windows (CMD):**

```cmd
.\project.exe --cycles 1000 requests.csv
```

Dieser Befehl startet die Simulation mit maximal 1000 Simulationszyklen und verwendet die Speicherzugriffe aus der Datei `requests.csv`.

## 3. Verfügbare Parameter

Mit dem folgenden Befehl werden alle verfügbaren Optionen angezeigt:

```bash
./project --help
```

| Parameter | Beschreibung |
|---|---|
| `--cycles` | Anzahl der Simulationszyklen |
| `--tf` | Pfad zur Trace-Datei |
| `--num-cache-levels` | Anzahl der Cache-Ebenen (1–3) |
| `--cacheline-size` | Größe einer Cachezeile in Bytes |
| `--num-lines-l1` | Anzahl der L1-Cachezeilen |
| `--num-lines-l2` | Anzahl der L2-Cachezeilen |
| `--num-lines-l3` | Anzahl der L3-Cachezeilen |
| `--latency-cache-l1` | Latenz des L1-Caches in Zyklen |
| `--latency-cache-l2` | Latenz des L2-Caches in Zyklen |
| `--latency-cache-l3` | Latenz des L3-Caches in Zyklen |
| `--mapping-strategy` | Mapping-Strategie: `0` = Direct-Mapped, `1` = Fully Associative |
| `--help` | Zeigt alle verfügbaren Optionen an |

Die Eingabedatei wird als letztes, positionsabhängiges Argument übergeben.

## 4. Beispiele

### Simulation mit drei Cache-Ebenen

```bash
./project --cycles 1000 --num-cache-levels 3 requests.csv
```

### Direct-Mapped Cache

```bash
./project --cycles 1000 --mapping-strategy 0 requests.csv
```

### Fully Associative Cache

```bash
./project --cycles 1000 --mapping-strategy 1 requests.csv
```

### Individuelle Cache-Latenzen

```bash
./project --cycles 1000 \
  --latency-cache-l1 1 \
  --latency-cache-l2 5 \
  --latency-cache-l3 20 \
  requests.csv
```

### Individuelle Cache-Konfiguration

```bash
./project --cycles 1000 \
  --num-cache-levels 3 \
  --cacheline-size 64 \
  --num-lines-l1 32 \
  --num-lines-l2 128 \
  --num-lines-l3 512 \
  --mapping-strategy 1 \
  requests.csv
```

### Simulation mit Trace-Datei

```bash
./project --cycles 1000 --tf trace.vcd requests.csv
```

Die Trace-Datei enthält die wichtigsten Signale der Simulation.

## 5. Format der Eingabedatei

Die Eingabedatei muss im CSV-Format vorliegen und eine Header-Zeile enthalten.

Beispiel für `requests.csv`:

```csv
Type,Address,Data
W,0x0010,20
R,0x0010,
W,123,0x12
R,123,
```

Die Spalten haben folgende Bedeutung:

- **Type:** `W` für Schreibzugriff oder `R` für Lesezugriff.
- **Address:** Speicheradresse in Dezimal- oder Hexadezimaldarstellung.
- **Data:** Zu schreibender Wert. Bei Lesezugriffen bleibt dieses Feld leer.

Bei jedem Zugriff werden vier Bytes ab der angegebenen Speicheradresse gelesen bzw. geschrieben.


**Hinweis:** Die Windows-Befehle setzen voraus, dass das Projekt für Windows kompiliert wurde und eine `project.exe` vorhanden ist. Unter Linux, macOS und WSL wird die ausführbare Datei `project` mit `./project` aufgerufen.
