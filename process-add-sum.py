# -*- coding: utf-8 -*-
"""
Created on Sun Jun 18 20:22:36 2017

@author: paul
"""

fp = open("/home/paul/Desktop/in-data-60mps.txt", 'r')

fw = open("/home/paul/Desktop/sum-1mps.txt", 'w')

flines = fp.readlines()

a = []

for line in flines:
    lineList = line.split('\t')
    a.append(lineList)

b = []


for i in range(len(a)):
    temp = []
    temp.append(i+1)
    j = 0
    sum = float(a[i][1])
    while j < i:
        sum += float(a[j][1])
        j += 1
    temp.append(sum)
    b.append(temp)

for k in b:
    fw.writelines(str(k[0]) + "\t" + str(k[1]) + "\n")        
    