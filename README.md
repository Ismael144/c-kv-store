# Simple Key Value Store

This is a simple key value store built in c with disk persistent

It uses Write Ahead Log(WAL) for disk persistence ensuring data is not lost  

## How to run 
Make sure you have gcc installed on your system
```bash 
gcc main.c -o main && ./main
```

## Supported Commands 
#### SET Command 
Set key to given value 
```bash 
SET <key> <value>
```

#### GET Command 
Get value by key from kv store
```bash 
GET <key>
```

#### DELETE Command 
Delete value by key
```bash 
DEL <key>
```