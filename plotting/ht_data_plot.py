import os
import csv
import pandas as pd

arr = os.listdir('data')
print(arr)
capacities = []
load_factors = []
insert_rates = []
lookup_rates = []
del_rates = []
for filename in arr:
    if filename == "sample.txt":
        continue
    d_flag = filename[0] == 'd' # flag indicating if deletion was tested

    data = pd.read_csv("data/" + filename, sep="\t")

    capacities.append(data["Capacity"].tolist())
    load_factors.append(data["LoadFactor"].tolist())
    insert_rates.append([row.InsertNumKVs/row.InsertTime / 1000.0 for row in data.iloc])
    lookup_rates.append([row.LookupNumKVs/row.LookupTime / 1000.0 for row in data.iloc])
    if d_flag:
        del_rates.append([row.DeleteNumKVs/row.DeleteTime / 1000.0 for row in data.iloc])

