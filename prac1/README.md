# Practice 1

Using basic Linux socket APIs and `poll`, `sig_actions` to learn basic components.

## Normal

Just a smallest practice of socket programming.

```bash 
cd prac1
make normal
./server &
./client
```

## IO multiplexing

Server needs to listen to two clients, and client needs to handle server and user input.

```bash 
cd prac1
make multi
./mul_server &
./mul_client
```

## Safe connection

Using some signals and flags to prevent from error causing exit.
Such as half-close.

```bash 
cd prac1
make safe
./safe_server &
./safe_client
```


