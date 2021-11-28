# Question - 1
## How to Run
```
cd q1
make
./q1
```
## Assumption
* Tutorial can be started with 0 student. To prevent all tutorials from happening with 0 students, appropriate delays have been added in the simulation.
## Some Points
* I have created threads for each course and for each student.
* The course thread is spawned first, since the courses wait for a period to get students. After all the threads corresponding to courses are created, the threads corresponding to students are spawned.
* I have created structs for courses, students and labs, and stored all of them in global arrays.
* I have used `mutex locks` and `pthread_cond_wait`, `pthread_cond_broadcast` alongside `pthread`s to simulate concurrency in the assignment.

## Course Thread
* The course thread first converts the `void*` argument to the course struct that is passed to the thread.
* Then, I run a while loop until the course registrations continue. All work related to the course happens in this loop, and the thread exits when this loop ends.
* I sleep at the start to give time for students time to fill preferences before TAs are chosen. I also set number of student slots available for that course's tutorial to 0 after acquiring the lock of the course.
* Then, I iterate over all the TAs of all the labs, checking if they are available to take the tutorial(after applying appropriate locks), and changing their tutorial counts/availability when I choose them. **BONUS IS IMPLEMENTED:** For each lab, I kept a check of the number of TAs who are yet to conduct the tutorial `i+1`th number of times, and only choose the current TA if it is one of them. Note that I acquire the lock of the TA before changing different values for him.
* If I don't find a TA, I check if there is a TA who is busy, but can take a tutorial. If no such TA exists, then I close the registrations of the course and the while loop exits. **The code also broadcasts to the students waiting for that course's tutorial to be held, and so, they can break and move on to their next preference/exit the simulation.** 
* Now, since I have found a TA, I find the random number of slots between 1 and the max limit given, and acquire the course lock and update them.
* **After allocating seatch, I broadcast on the condition variable of that course that signals to the students waiting for the course's slots to start filling that the slot filling has begun.**
```
pthread_cond_broadcast(&cour->tut_slots_condn);
```
* After this, the code sleeps so that the students may fill in their preferences.
* After this, the tutorial begins.
* The code again sleeps for the duration for which the tutorial is conducted.
* After this, the thread broadcasts a signal on `tut_session_cond` condition variable, so that the students that were sleeping on that course's tutorial wake up and continue.
* After this, the TA is freed, and the loop continues to conduct another tutorial for this course.

## Students thread
* First, I obtained the student struct from the void* argument.
* Then, I made the thread sleep for the waiting time of the student.
* Then, I iterated over all the three course preferences of the student.
* Then, the thread waits on the condition variable `tuts_slots_condn` of that specific chosen course. It does this waiting inside a while loop, and breaks only if the course has available slots or if that course's registrations have closed.
* If the course's registrations have closed, it moves on to the next preference.
* After this, the student gets allocated a seat in the course, since the condition variable has been released.
* Now, the student waits until the tutorial is finished. This is accomplished by doing a `pthread_cond_wait` on `course->tut_session_cond`, which gets a broadcast when the tutorial of that particular course gets over.
* After this, the code decides whether the student selects the course permanently or withdraws from the course. This choosing is done by finding a random value between 0 and 1, and if that value is less than the probability of accepting(b/w 0 and 1), then he accepts the course.
    * If the student accepts the course, then the thread returns and the student exits the simulation.
    * If the student rejects the course, then the loop moves on to the next preference. When all three preferences are iterated through, then too, the thread returns and the student is left unallocated for a course.