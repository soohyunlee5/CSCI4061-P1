# MapReduce

### Project Overview

MapReduce was originally created by Google to help efficiently compute their rapidly growing data on a distributed cluster of machines. MapReduce consists of two main steps, Map and Reduce, and one optional step that comes in between the first two main steps called Combine. 

### 1. Map
The first step, Map, takes an input key-value pair where the key represents an address and the value represents the entire record content. A map function will then process the inputs and output intermediate key-value pairs based on the logic defined by the user. For example, the key could represent a word and the value could represent a single occurrence. These intermediate key-value pairs will then be sent to be shuffled and sorted so that all values associated with the same key will be sent to the same reducer.

### 2. Combine (Optional)
The optional combine step is mainly used for the optimization of the MapReduce process. This step, if implemented, usually runs after Map and performs a local aggregation of the intermediate key-value pairs on each machine before being sent to be shuffled and sorted. This optional step reduces the amount of data needed to transfer between machines allowing improved efficiency and performance.

### 3. Reduce


### Assumptions

Write any assumptions that we need to know about running your MapReduce implementation, or grading here.

### Work Split

### PA1 - Intermediate Submission

### Jake Kim 
Wrote Reduce and part of Combine sections

### Soohyun Lee
Wrote Map and part of Combine sections

### AI Use

No AI was used for this assignment
