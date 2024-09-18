import re
import matplotlib.pyplot as plt
import numpy as np

# Specify the path to your log file and the output file
input_file_path = 'output.txt'
output_rand_path = 'extracted_rand_values.txt'
output_time_path = 'timestamp.txt'
plot_output_path = 'histogram.png'  # Specify the output file for the plot
# Read the log data from the input file
with open(input_file_path, 'r') as file:
    log_data = file.read()

# Regular expression to match the pattern and extract the random exponential value
pattern_time = r'DEBUG: An event is added: Event: arrival / dest= \d+ / time = ([\d.]+)'

pattern_rand = r'DEBUG: rand exp value: ([\d.]+)'

# Find all matching lines and extract the values
rand_values = re.findall(pattern_rand, log_data)
time_values = re.findall(pattern_time, log_data)

# Write the extracted random values to the output file
with open(output_rand_path, 'w') as output_rand:
    for value in rand_values:
        output_rand.write(value + '\n')

with open(output_time_path, 'w') as output_time:
    for time in time_values:
        output_time.write(time+ '\n')


print(f"Extracted random values have been stored in {output_rand_path}")
print(f"Extracted timestamp have been stored in {output_time_path}")

# Convert the extracted values to floats for plotting
rand_values = list(map(float, rand_values))

# Plot the histogramnum_bins = 30  # For example, 30 bins, you can adjust this as needed
num_bins=20

# Create a histogram with proper binning for float values
counts, bins, patches = plt.hist(rand_values, bins=np.linspace(min(rand_values), max(rand_values), num_bins), density=True, edgecolor='black')
plt.xlabel('Random Poisson Value')
plt.ylabel('Frequency')
plt.title('Normalized Histogram of Poisson Distribution')
plt.savefig(plot_output_path)
print(f"Histogram saved as {plot_output_path}")
bin_widths = np.diff(bins)
total_area = np.sum(counts * bin_widths)
print(f"Total area under the histogram: {total_area}")