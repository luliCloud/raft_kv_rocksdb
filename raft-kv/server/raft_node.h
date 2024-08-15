#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include <vector>
#include <raft-kv/transport/transport.h>
#include <raft-kv/raft/node.h>
#include <raft-kv/server/redis_store.h>
#include <raft-kv/wal/wal.h>
#include <raft-kv/snap/snapshotter.h>
#include <rocksdb/db.h>  // for rocksdb

namespace kv {

class RaftNode : public RaftServer {
 public:
  static void main(uint64_t id, const std::string& cluster, uint16_t port);

  explicit RaftNode(uint64_t id, const std::string& cluster, uint16_t port);

  ~RaftNode() final;

  void deleteDatabase();

  void stop();

  void propose(std::shared_ptr<std::vector<uint8_t>> data, const StatusCallback& callback);

  void process(proto::MessagePtr msg, const StatusCallback& callback) final;

  void is_id_removed(uint64_t id, const std::function<void(bool)>& callback) final;

  void report_unreachable(uint64_t id) final;

  void report_snapshot(uint64_t id, SnapshotStatus status) final;

  uint64_t node_id() const final { return id_; }

  bool publish_entries(const std::vector<proto::EntryPtr>& entries);
  void entries_to_apply(const std::vector<proto::EntryPtr>& entries, std::vector<proto::EntryPtr>& ents);
  void maybe_trigger_snapshot();

 private:
  void start_timer();
  void pull_ready_events();
  Status save_snap(const proto::Snapshot& snap);
  void publish_snapshot(const proto::Snapshot& snap);

  // replay_WAL replays WAL entries into the raft instance.
  void replay_WAL();
  // open_WAL opens a WAL ready for reading.
  void open_WAL(const proto::Snapshot& snap); // open the WAL based on snap as start point

  void schedule();

  uint16_t port_;
  pthread_t pthread_id_;
  boost::asio::io_service io_service_;
  boost::asio::deadline_timer timer_;
  uint64_t id_;
  std::vector<std::string> peers_;
  uint64_t last_index_;
  proto::ConfStatePtr conf_state_;
  uint64_t snapshot_index_;
  // 已应用的索引是系统已经应用到状态记的最新日志条目的索引。换句话说，这是系统当前状态对应的的最新
  // 日志条目的索引。所有索引小于或等于已应用索引的日志条目已经被系统处理并反映在当前的系统状态中。
  uint64_t applied_index_; 

  MemoryStoragePtr storage_;
  std::unique_ptr<Node> node_;
  TransporterPtr transport_;
  std::shared_ptr<RedisStore> redis_server_; // 指向RedisStore的指针

  std::vector<uint8_t> snap_data_;
  std::string snap_dir_;
  uint64_t snap_count_;
  std::unique_ptr<Snapshotter> snapshotter_;

  std::string wal_dir_;
  WAL_ptr wal_;

  std::string rocksdb_dir_;  // lu: for RocksDb
  rocksdb::DB* db_;
};
typedef std::shared_ptr<RaftNode> RaftNodePtr;

}