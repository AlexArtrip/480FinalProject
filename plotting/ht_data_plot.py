import math
import os
from collections import defaultdict
import csv
import pandas as pd

arr = os.listdir('data')
print(arr)
names = []
capacities_l = defaultdict(list)
load_factors_l = defaultdict(list)
insert_rates_l = defaultdict(list)
lookup_rates_l = defaultdict(list)
del_rates_l = defaultdict(list)

capacities_c = defaultdict(list)
load_factors_c = defaultdict(list)
insert_rates_c = defaultdict(list)
lookup_rates_c = defaultdict(list)
del_rates_c = defaultdict(list)
# capacities = defaultdict(list)
# load_factors = defaultdict(list)
# insert_rates = defaultdict(list)
# lookup_rates = defaultdict(list)
# del_rates = defaultdict(list)
for filename in arr:
    if filename == "sample.txt" or filename == ".DS_Store":
        continue
    d_flag = filename[0] == 'd' # flag indicating if deletion was tested
    name = filename.split("_")[0]
    names.append(name)

    data = pd.read_csv("data/" + filename, sep="\t")

    cap_temp_l = []
    load_temp_l = []
    ins_temp_l = []
    look_temp_l = []
    del_temp_l = []

    cap_temp_c = []
    load_temp_c = []
    ins_temp_c = []
    look_temp_c = []
    del_temp_c = []

    for row in data.iloc:
        if math.log2(row.Capacity) == 26:
            cap_temp_c.append(math.log2(row.Capacity))
            load_temp_c.append(row.LoadFactor)
            ins_temp_c.append(row.InsertNumKVs/row.InsertTime / 1000.0)
            look_temp_c.append(row.LookupNumKVs/row.LookupTime / 1000.0)
            if d_flag:
                del_temp_c.append(row.DeleteNumKVs/row.DeleteTime / 1000.0)

        if row.LoadFactor == 0.5:
            cap_temp_l.append(math.log2(row.Capacity))
            load_temp_l.append(row.LoadFactor)
            ins_temp_l.append(row.InsertNumKVs/row.InsertTime / 1000.0)
            look_temp_l.append(row.LookupNumKVs/row.LookupTime / 1000.0)
            if d_flag:
                del_temp_l.append(row.DeleteNumKVs/row.DeleteTime / 1000.0)

    capacities_c[name].extend(cap_temp_c)
    load_factors_c[name].extend(load_temp_c)
    insert_rates_c[name].extend(ins_temp_c)
    lookup_rates_c[name].extend(look_temp_c)
    if d_flag:
        del_rates_c[name].extend(del_temp_c)

    capacities_l[name].extend(cap_temp_l)
    load_factors_l[name].extend(load_temp_l)
    insert_rates_l[name].extend(ins_temp_l)
    lookup_rates_l[name].extend(look_temp_l)
    if d_flag:
        del_rates_l[name].extend(del_temp_l)

import matplotlib.pyplot as plt
plt.style.use('seaborn-whitegrid')
import numpy as np

plt.title("Insertion LF vs Speed with Capacity 2^26")
plt.xlabel("Load Factor")
plt.ylabel("Insert Rate (mil keys/sec)")
for name in names:
    plt.plot(load_factors_c[name], insert_rates_c[name], label = name)
plt.legend(loc="upper right")
plt.savefig("insert_LFvSpeed.png")

plt.clf()
plt.title("Lookup LF vs Speed with Capacity 2^26")
plt.xlabel("Load Factor")
plt.ylabel("Lookup Rate (mil keys/sec)")
for name in names:
    plt.plot(load_factors_c[name], lookup_rates_c[name], label = name)
plt.legend(loc="upper right")
plt.savefig("lookup_LFvSpeed.png")

plt.clf()
plt.title("Insertion Capacity vs Speed with LF 0.5")
plt.xlabel("Capacity (log 2)")
plt.ylabel("Insert Rate (mil keys/sec)")
for name in names:
    plt.plot(capacities_l[name], insert_rates_l[name], label = name)
plt.legend(loc="upper right")
plt.savefig("insert_CvSpeed.png")

plt.clf()
plt.title("Lookup Capacity vs Speed with LF 0.5")
plt.xlabel("Capacity (log 2)")
plt.ylabel("Lookup Rate (mil keys/sec)")
for name in names:
    plt.plot(capacities_l[name], lookup_rates_l[name], label = name)
plt.legend(loc="upper right")
plt.savefig("lookup_CvSpeed.png")