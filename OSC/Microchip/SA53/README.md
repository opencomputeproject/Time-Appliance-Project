# Timetickler

A utility to query and change MAC parameters.

## Install

Clone respository and install the requirements,

`pip install -r requirements.txt`

Using `pyinstaller` create an executable,

`pyinstaller --onefile timetickler.py`

This will create a binary file `timetickler` in `./dist/`, you may copy this file to another directory,

`cp ./dist/timetickler /my/directory/here`

## Usage
To list timecard information populated in `sysfs`

```
./timetickler list
```

To query a field,

```
./timetickler get Phase
```

Multiple fields can be queried at the same time,

```
./timetickler get Phase LastCorrection PpsOffset
```

By default, `timetickler` will output the result of one query, to specify the numer of queries use the `-n` flag,

```
# output 10 readings
./timetickler get Phase -n 10
```

To query a field or fields until terminated by user specify `-1`,

```
./timetickler get Phase -n -1
```

To set a field,

```
./timetickler set PpsOffset 1234
```

To set a field and have the value persist after timecard reset, store the value,

```
./timetickler set PpsOffset 1234 --store
```

