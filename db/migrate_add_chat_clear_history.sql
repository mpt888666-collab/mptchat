-- 已有 chatdb 的增量升级脚本：新增「按用户清空聊天记录」的水位表
-- 新库直接导入 db/schema.sql 即可，无需执行本文件
CREATE TABLE IF NOT EXISTS `chat_clear_history` (
  `user_id` bigint unsigned NOT NULL COMMENT '清空操作发起人',
  `thread_id` bigint unsigned NOT NULL COMMENT '会话 id',
  `cleared_before` bigint unsigned NOT NULL DEFAULT '0' COMMENT '小于等于该 message_id 的消息对该用户不可见',
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`user_id`,`thread_id`),
  KEY `idx_thread_user` (`thread_id`,`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
