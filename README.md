# __Tanks Project__

### _Author_: Livio Bisogni
###### __&copy; 2021 REAL-TIME INDUSTRY Inc.__
___
Is that water?

## Prerequisites

* [Allegro 4](https://liballeg.org/stabledocs/en/index.html) - A cross-platform, open-source graphics library for game and multimedia programming. To install Allegro 4.4 under Ubuntu (or derivatives thereof), execute these commands from the terminal:

	```bash
	sudo apt update
	sudo apt install liballegro4.4 liballegro4-dev allegro4-doc
	```
* [MyPtask](https://github.com/kimjong0xff/myptask) - An educational real-time library.

## How to compile

* To compile the project the first time:

	1. Move the `tanksimulator-main` folder (from now on referred to as `tanksimulator-main/`) where thou prefer.
	2. Launch a terminal window and navigate to `tanksimulator-main/`.
	3. Create the `obj` folder and move it inside `tanksimulator-main/`, i.e., `tanksimulator-main/obj/`.
	4. Type:

    	```bash
    	make
    	```

* To compile the project again:

	1. Navigate to `tanksimulator-main/`.
	2. Type:

    	```bash
    	make
    	```

* To compile the project from scratch:

	1. Navigate to `tanksimulator-main/`.
	2. Type:

    	```bash
    	make clean
    	make
    	```

## How to execute

The compilation produces the executable file `tanksimulator-main/main`. The program should be run as superuser (i.e., root user), e.g.,

```bash
sudo ./main
# Type your super secret password
```

## Nope, that's _magic_ water!
Let N, N = 5, be the number of tanks. Hence, there are n = 2 * (N + 1) = 12 periodic tasks:

* &tau;<sub>T,1</sub>, ..., &tau;<sub>T,N</sub> tank tasks;
* &tau;<sub>S,1</sub>, ..., &tau;<sub>S,N</sub> sensor tasks;
* &tau;<sub>U</sub> user task;
* &tau;<sub>G</sub> graphics task.

This figure shows the tasks-resources diagram:

![](img/Tasks-resources_diagram.png)

You can find more about this project on the attached [pdf](project-report.pdf). Yeah, that's available in Italian only, though (sorry folks).

Finally, here are some screenshots of the GUI:

![](img/t.png)

<p align="center" width="100%">
    <img width="61.8%" src="img/t2.png"> 
</p>

<p align="center" width="100%">
    <img width="38.2%" src="img/t3.png"> 
</p>

<p align="center" width="100%">
    <img width="38.2%" src="img/t4.png"> 
</p>

<p align="center" width="100%">
    <img width="38.2%" src="img/t5.png"> 
</p>
