# -*- coding: utf-8 -*-
"""
Created on Sat Jun 17 15:11:18 2017

@author: paul
"""

fp = open("rate-trace.txt", 'r')

fw = open("/home/paul/Desktop/in-data-60mps.txt", 'w')

flines = fp.readlines()

node = "0"

dataType = "InData"

#fw.writelines(flines[0])

a = []

for line in flines[1:]:
    listInLine = line.split('\t')
    if len(listInLine) > 4:
        if listInLine[4] == dataType and listInLine[1] == node and \
        (listInLine[3] == "netdev://[00:00:00:00:00:01]" or \
        listInLine[3] == "netdev://[00:00:00:00:00:05]"):
            #fw.writelines(listInLine)
            temp = []
            temp.append(listInLine[0])
            temp.append(listInLine[6])
            a.append(temp)


print "here"
new = []
new.append(a[0])
i = 0
counter = 0
while i < len(a):
    if i == 0:
        i += 1
        continue
    new.append(a[i])
    counter += 1
    if i + 1 != len(a):
        if a[i][0] == a[i + 1][0]:
            print a[i + 1][0]
            temp = float(a[i][1]) + float(a[i + 1][1])
            new[counter][1] = str(temp)
            i += 1
    i += 1
            
for i in new:
    s = i[0] + '\t' + i[1] + '\n'
    fw.writelines(s)