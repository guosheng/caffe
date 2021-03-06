// Copyright 2013 Yangqing Jia
#include <glog/logging.h>
#include <leveldb/db.h>
#include <stdint.h>

#include <string>

#include "caffe/proto/caffe.pb.h"
#include "caffe/util/io.hpp"

using caffe::Datum;
using caffe::BlobProto;

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  if (argc != 3) {
    LOG(ERROR) << "Usage: demo_compute_image_mean input_leveldb output_file";
    return(0);
  }

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = false;

  LOG(INFO) << "Opening leveldb " << argv[1];
  leveldb::Status status = leveldb::DB::Open(
      options, argv[1], &db);
  CHECK(status.ok()) << "Failed to open leveldb " << argv[1];

  leveldb::ReadOptions read_options;
  read_options.fill_cache = false;
  leveldb::Iterator* it = db->NewIterator(read_options);
  it->SeekToFirst();
  Datum datum;
  BlobProto sum_blob;
  int count = 0;
  datum.ParseFromString(it->value().ToString());
  sum_blob.set_num(1);
  sum_blob.set_channels(datum.channels());
  sum_blob.set_height(datum.height());
  sum_blob.set_width(datum.width());
  const int data_size = datum.channels() * datum.height() * datum.width();
  for (int i = 0; i < datum.data().size(); ++i) {
    sum_blob.add_data(0.);
  }
  LOG(INFO) << "Starting Iteration";
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    // just a dummy operation
    datum.ParseFromString(it->value().ToString());
    const string& data = datum.data();
    CHECK_EQ(data.size(), data_size) << "Incorrect data field size " << data.size();
    for (int i = 0; i < data.size(); ++i) {
      sum_blob.set_data(i, sum_blob.data(i) + (uint8_t)data[i]);
    }
    ++count;
    if (count % 10000 == 0) {
      LOG(ERROR) << "Processed " << count << " files.";
    }
  }
  for (int i = 0; i < sum_blob.data_size(); ++i) {
    sum_blob.set_data(i, sum_blob.data(i) / count);
  }
  // Write to disk
  LOG(INFO) << "Write to " << argv[2];
  WriteProtoToBinaryFile(sum_blob, argv[2]);

  delete db;
  return 0;
}
