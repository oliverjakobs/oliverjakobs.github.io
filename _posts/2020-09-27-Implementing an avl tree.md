---
layout: post
author: oliver
lang: C
---
Iterative method to implement an avl tree in c.

>Disclaimer: This is not the only or best way to do this. This is how I've done it. Maybe it helps with your project or maybe you get inspired to do something better.

Before we start we need to talk about what an avl tree even is and why we may want to use it. An AVL tree (named after inventors Adelson-Velsky and Landis) is a self-balancing binary search tree. Lookup, insertion, and deletion all take O(log n) time in both the average and worst cases (with n beeing the number of nodes prior to the operation). After insertion and deletion the tree may require to be rebalanced by one or two tree rotations.

AVL trees are often compared with red–black trees because both support the same set of operations and take O (log ⁡n) time for the basic operations. For lookup-intensive applications, AVL trees are faster than red–black trees because they are more strictly balanced.
