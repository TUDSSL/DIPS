import h5py
import sys

extension = '.h5'
filename = sys.argv[1]
if extension in filename:
  filename = filename.replace(extension, '')

  # Create HDF5 file and dump content of other file with gzip compression (not lzf)
  with h5py.File((filename + '.h5'), "r") as h5f:
      with h5py.File((filename + '-conv.h5'), "w") as h5fnew:
          arr = h5f["data"]["voltage"][:]
          h5fnew.create_dataset("data/voltage", data=arr, compression="gzip")
          h5fnew['data']['voltage'].attrs['gain'] = h5f['data']['voltage'].attrs['gain']
          h5fnew['data']['voltage'].attrs['offset'] = h5f['data']['voltage'].attrs['offset']
          h5fnew['data']['voltage'].attrs['description'] = h5f['data']['voltage'].attrs['description']
          h5fnew['data']['voltage'].attrs['unit'] = h5f['data']['voltage'].attrs['unit']

          arr = h5f["data"]["time"][:]
          h5fnew.create_dataset("data/time", data=arr, compression="gzip")
          h5fnew['data']['time'].attrs['description'] = h5f['data']['time'].attrs['description']
          h5fnew['data']['time'].attrs['unit'] = h5f['data']['time'].attrs['unit']

          arr = h5f["data"]["current"][:]
          h5fnew.create_dataset("data/current", data=arr, compression="gzip")
          h5fnew['data']['current'].attrs['gain'] = h5f['data']['current'].attrs['gain']
          h5fnew['data']['current'].attrs['offset'] = h5f['data']['current'].attrs['offset']
          h5fnew['data']['current'].attrs['description'] = h5f['data']['current'].attrs['description']
          h5fnew['data']['current'].attrs['unit'] = h5f['data']['current'].attrs['unit']
          
else:
  print("Missing or invalid filename (*.h5)")
