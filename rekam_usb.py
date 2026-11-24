    import os
import sys
import time
import threading
import serial
import serial.tools.list_ports

def pick_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("[ERR] Tidak ada port COM terdeteksi! Pastikan kabel USB ESP32 terpasang.")
        sys.exit(1)
    print("\nPort COM yang tersedia:")
    for idx, p in enumerate(ports):
        print(f"  [{idx}] {p.device} - {p.description}")
    if len(ports) == 1:
        print(f"Otomatis memilih {ports[0].device}")
        return ports[0].device
    sel = input("Pilih nomor port [default 0]: ").strip()
    idx = int(sel) if sel.isdigit() and int(sel) < len(ports) else 0
    return ports[idx].device

def main():
    print("============================================================")
    print("  PEREKAM DATASET ESP32 VIA USB -> FILE CSV LAPTOP (50 Hz)")
    print("============================================================")

    port = pick_port()
    subjek = input("Masukkan ID Subjek [default S08]: ").strip() or "S08"
    posisi = input("Posisi alat (pinggang / pergelangan) [default pinggang]: ").strip().lower() or "pinggang"
    if posisi not in ("pergelangan", "pinggang"):
        print(f"[INFO] Menggunakan posisi: {posisi}")
    csv_path = input("Nama file CSV output [default dataset_usb.csv]: ").strip() or "dataset_usb.csv"

    file_exists = os.path.exists(csv_path) and os.path.getsize(csv_path) > 0
    csv_file = open(csv_path, "a", encoding="utf-8", newline="\n")
    if not file_exists:
        csv_file.write("timestamp_ms,subjek,sesi,posisi,ax,ay,az,label\n")
        csv_file.flush()

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.2
    ser.dtr = False
    ser.rts = False
    try:
        ser.open()
    except serial.SerialException as e:
        print(f"\n[ERR] Gagal membuka {port}: {e}")
        print(">> Pastikan Serial Monitor & Arduino IDE sudah ditutup, lalu cabut-colok kabel USB ESP32.")
        csv_file.close()
        sys.exit(1)
    time.sleep(1.5)
    ser.reset_input_buffer()

    stop_flag = threading.Event()
    total_saved = [0]
    session_saved = [0]

    def reader_loop():
        while not stop_flag.is_set():
            try:
                raw = ser.readline().decode("utf-8", errors="ignore").strip()
                if not raw:
                    continue
                if raw.startswith("DATA,"):
                    # DATA,ts_ms,sesi,ax,ay,az,label
                    parts = raw.split(",")
                    if len(parts) == 7:
                        _, ts_ms, sesi, ax, ay, az, label = parts
                        csv_file.write(f"{ts_ms},{subjek},{sesi},{posisi},{ax},{ay},{az},{label}\n")
                        total_saved[0] += 1
                        session_saved[0] += 1
                        if session_saved[0] % 50 == 0:
                            csv_file.flush()
                            print(f"  [REC {label}] {session_saved[0]} sampel ({session_saved[0]/50:.0f} dtk) | ax={ax} ay={ay} az={az}")
                elif raw.startswith("STATUS,"):
                    print(f"[ESP32] {raw}")
                    if raw.startswith("STATUS,START"):
                        session_saved[0] = 0
                    elif raw == "STATUS,ALL_DONE":
                        csv_file.flush()
                        print(f"\n>>> SELESAI! Total tersimpan ke '{csv_path}': {total_saved[0]} baris <<<")
                else:
                    # Tampilkan output lain (misal jika ESP32 masih menjalankan sketch lama)
                    print(f"[ESP32-RAW] {raw}")
            except serial.SerialException as e:
                if not stop_flag.is_set():
                    print(f"\n[ERR] Koneksi USB terputus ({e}).")
                    print(">> Cek kabel USB apakah longgar/tersenggol, lalu cabut-colok & jalankan ulang.")
                    stop_flag.set()
                break
            except Exception as e:
                if not stop_flag.is_set():
                    print(f"[WARN] Serial read error: {e}")
                    time.sleep(0.2)

    t = threading.Thread(target=reader_loop, daemon=True)
    t.start()

    print("\nPerintah yang tersedia:")
    print("  a = Rekam OTOMATIS berurutan (diam 60d -> jalan 60d -> lari 20d)")
    print("  d = Rekam manual DIAM  (tekan 's' untuk berhenti)")
    print("  j = Rekam manual JALAN (tekan 's' untuk berhenti)")
    print("  l = Rekam manual LARI  (tekan 's' untuk berhenti)")
    print("  s = Stop rekaman saat ini")
    print("  q = Keluar & simpan file CSV\n")

    try:
        while not stop_flag.is_set():
            cmd = input("Ketik perintah (a / d / j / l / s / q): ").strip().lower()
            if stop_flag.is_set():
                break
            if cmd == "q":
                try:
                    ser.write(b"s")
                except Exception:
                    pass
                time.sleep(0.2)
                break
            elif cmd in ("a", "d", "j", "l", "s"):
                try:
                    ser.write(cmd.encode("ascii"))
                except serial.SerialException:
                    print("[ERR] Gagal kirim perintah: koneksi USB terputus.")
                    break
            else:
                print("Perintah tidak dikenal. Gunakan a, d, j, l, s, atau q.")
    except KeyboardInterrupt:
        try:
            ser.write(b"s")
        except Exception:
            pass
        time.sleep(0.2)
    finally:
        stop_flag.set()
        t.join(timeout=1.0)
        csv_file.flush()
        csv_file.close()
        ser.close()
        print(f"\nFile tersimpan aman di: {os.path.abspath(csv_path)} ({total_saved[0]} baris baru).")

if __name__ == "__main__":
    main()
