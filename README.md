# Pub/Sub en C (TCP y UDP) — Guía completa para Windows con WSL + VS Code

## 1) Instalar WSL y Ubuntu

1. Abre **PowerShell como Administrador** y ejecuta:
   'wsl --install -d Ubuntu'
   Si ya existe: verás 'ERROR_ALREADY_EXISTS'. En ese caso, simplemente abre “Ubuntu” desde el menú inicial o use el comando 'wsl -d Ubuntu' para arrancarlo.
   Verifica WSL2:
   'wsl --status'
   'wsl --set-default-version 2'

2. Abre Ubuntu (primera vez te pedirá crear usuario/clave).
   En la terminal de Ubuntu, ingresa los siguientes comandos:
   'sudo apt update'
   'sudo apt install -y build-essential gdb make zip unzip'

## 2) Abrir proyecto en VS Code - WSL y Compilación

1. En VS Code, instala la extensión **Remote - WSL** y **C/C++**

2. En la barra de búsqueda superior, ingresa **'>Open Folder in WSL'** y abre la carpeta donde haya descargado el repositorio

3. En la terminal de VS Code (WSL: Ubuntu), ingresa el siguiente comando para compilar el código:
   'make clean && make'

## 3) Ejecutar TCP

1. Terminal 1 (broker): Ingresar comando './broker_tcp 5000'

2. Terminal 2 (suscriptor): Ingresar comando './subscriber_tcp 127.0.0.1 5000 "EQUIPO_A" "EQUIPO_B"'

3. Terminal 3 (publicador): Ingresar comando './publisher_tcp 127.0.0.1 5000 "EQUIPO_A" 10'

(Los valores de los puertos, la IP del broker y la cantidad de mensajes enviados pueden ser diferentes, simplemente hicimos las pruebas con estos).

## 4) Ejecutar UDP

1. Terminal 1 (broker): Ingresar comando './broker_udp 6000'

2. Terminal 2 (suscriptor): Ingresar comando './subscriber_udp 127.0.0.1 6000 "EQUIPO_A" "EQUIPO_B"'

3. Terminal 3 (publicador): Ingresar comando './publisher_udp 127.0.0.1 6000 "EQUIPO_A" 10'

(Los valores de los puertos, la IP del broker y la cantidad de mensajes enviados pueden ser diferentes, simplemente hicimos las pruebas con estos).



