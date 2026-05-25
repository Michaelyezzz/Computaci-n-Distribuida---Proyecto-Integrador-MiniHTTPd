# MiniHTTPd

Servidor HTTP/1.1 básico en C usando epoll para manejo asincrónico de conexiones.

## Características

- **Arquitectura event-driven** con epoll
- **Conexiones persistentes** (Keep-Alive)
- **Validación de seguridad** contra directory traversal
- **Tipos MIME** automáticos
- **Códigos de estado HTTP** completos
- **Soporte para múltiples clientes concurrentes**

## Compilación

```bash
make
```

## Ejecución

```bash
./minihttpd 8080
```

## Estructura

- `src/` - Código fuente
- `include/` - Headers
- `www/` - Archivos estáticos

## Pruebas

```bash
curl http://localhost:8080/
curl http://localhost:8080/style.css
curl -X POST http://localhost:8080/  # Debe dar 405
curl -i --path-as-is http://localhost:8080/../../etc/passwd  # Debe rechazar
```