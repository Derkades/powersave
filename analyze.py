import csv
import pandas

data = pandas.read_csv('measurements.csv')
print(data.groupby('partition point beide'))

# with open('measurements.csv', 'r') as measurements_file:
#     rows = csv.reader(measurements_file, delimiter=',')
#     for row in rows:
#         print(row)