# Analysis

![image](https://user-images.githubusercontent.com/59967644/160084519-220a7ae1-7652-41d7-afc0-3706c09ff7f9.png)

### Notes on execution times
-	Execution times were manually measured for each task. 
-	Each task was first setup for extreme cases such that the measurement obtained would be the longest possible execution time for the task. The modifications made can be seen in the table above.
- Multiple iterations of execution times were also recorded and then divided by the number of iterations to find a more accurate execution time.
- The maximum execution time of decodeTask was found to be slightly higher than the value recorded in the table above, but this higher execution time is from the result of decoding a message that in practice cannot be received more than once in any full queue, hence the constraints for this execution time were relaxed slightly.

### Notes on initiation intervals
-	Some initiation intervals have been defined in the code and others were manually calculated.
-	Where given the option the time constraint of the hardware was chosen over the time constraint of the user. This is because for the critical instant analysis, the aim is to have the most extreme case.
-	The CAN interrupt intervals are 0.7ms as this time (the minimum length of a CAN frame) is shorter than the user extreme case which was approximately 51ms (as the fastest piano player can play 19.5 notes per second).

### Effect of queues
-	There are two queues used in the code, one by CAN_TX_Task and the other by decodeTask.
-	These queues give a task a lower worst-case utilization. 
-	The calculations for these queues are done internally in the table but are accounted for (as the queues are of length 36, both the initiation interval and maximum execution time were multiplied by 36, which reduces the priority of the tasks).

### Critical instant analysis
-	From the table above it can be seen that all tasks are executed within the interval of the longest task as:

![image](https://user-images.githubusercontent.com/59967644/160085210-a18923e3-c12f-4087-a0e5-113d7b317203.png)

-	Therefore, with the rate-monotonic scheduling setup as described above, CPU utilization is optimal.

### Quantification of total CPU usage
-	From the table above it can be seen that for the worst-case situation there is a 97.62% utilization of the CPU. 
-	This value is pretty optimal as most of the time CPU is being utilized with small leeway for safety.  
