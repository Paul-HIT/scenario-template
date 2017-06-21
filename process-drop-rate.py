# -*- coding: utf-8 -*-
"""
Created on Sat Jun 17 19:57:22 2017

@author: paul
"""

fp = open("rate-trace.txt", 'r')

ifw = open("/home/paul/Desktop/drop-rate-120mps-in.txt", 'w')

ofw = open("/home/paul/Desktop/drop-rate-120mps-out.txt", 'w')

flines = fp.readlines()

in_arr = []
out_arr = []

inCounter = 0
outCounter = 0

for line in flines[1:]:
    listInLine = line.split('\t')
    if len(listInLine) > 4:
	    if listInLine[4] == "InSatisfiedInterests" and listInLine[1] == "0" \
	    and listInLine[3] == "appFace://":
	        inCounter += float(listInLine[5])
	        ifw.writelines(line)
	    if listInLine[4] == "OutInterests" and listInLine[1] == "0" \
	    and listInLine[3] == "appFace://":
	        ofw.writelines(line)
	        outCounter += float(listInLine[5])

print "InInterest: " + str(inCounter)
print "OutInterest: " + str(outCounter)