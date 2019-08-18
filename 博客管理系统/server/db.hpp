//创建一些相关的类  封装数据库操作
#include<cstring>
#include<memory>
#include<cstdlib>
#include<cstdio>
#include<mysql/mysql.h>
#include<jsoncpp/json/json.h>

namespace Blog_System
{
  static MYSQL* mysqlinit()
  {
    //初始化一个mysql句柄
    //创建一个句柄
    MYSQL* connect_fd = mysql_init(NULL);
    //2.和数据库建立连接
    if (mysql_real_connect(connect_fd, "127.0.0.1","root","0303151616","blog_system1", 3306, NULL, 0) == NULL)
    {
      printf("连接失败 %s\n", mysql_error(connect_fd));
      return NULL;
    }
    //设置字符集编码
    mysql_set_character_set(connect_fd, "utf8");
    return connect_fd;
  }

  //释放句柄，断开连接
  static void mysqlrelase(MYSQL* connect_fd)
  {
    mysql_close(connect_fd);
  }


  //创建一个类，用于操作博客表的类
  class BlogTable
  {
    public:
      BlogTable(MYSQL* connect_fd)
        :mysql_(connect_fd)
      {
        //通过这个函数获取一个数据库操作句柄
        
      }

      //插入
      bool Insert(const Json::Value& blog)
      {
        const std::string& content = blog["content"].asString();
        std::unique_ptr<char> content_escape(new char[content.size() * 2 + 1]);
        //先把正文转移
        mysql_real_escape_string(mysql_, content_escape.get(), content.c_str(), content.size());
        std::unique_ptr<char> sql(new char[content.size() * 2 + 4096]);
        sprintf(sql.get(), "insert into blog_table values(null, '%s', '%s', %d,'%s')",
            blog["title"].asCString(), content_escape.get(), blog["tag_id"].asInt(), 
            blog["create_time"].asCString());
        int ret = mysql_query(mysql_, sql.get());
        if(ret != 0)
        {
          printf("执行sql失败！%s    %s\n", sql.get(), mysql_error(mysql_));
          return false;
        }

        return true;
      }

      //查找所有博客
      //blogs为输出行参数
      bool SelectAll(Json::Value* blogs, const std::string& tag_id = "")
      {
        char sql[1024 * 4] = {0};
        if(tag_id.empty())
        {
          sprintf(sql, "select blog_id, title, tag_id, create_time from blog_table");
        }
        else
        {
          sprintf(sql, "select blog_id, title, tag_id, create_time from blog_table where tag_id = '%s'", tag_id.c_str()); 
        }

        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行失败！%s\n", mysql_error(mysql_));
          return false;
        }
        
        //获取结果集合
        MYSQL_RES* result = mysql_store_result(mysql_);
        if(result == NULL)
        {
          printf("获取结果失败！%s\n", mysql_error(mysql_));
          return false;
        }
        //获取集合有多少行（意思是有多少篇博客）
        int rows = mysql_num_rows(result);
        for(int i = 0; i < rows; i++)
        {
          //获取具体行（一行就是一篇博客）
          MYSQL_ROW row = mysql_fetch_row(result);
          Json::Value blog;
          blog["blog_id"] = atoi(row[0]); //把c风格的字符串  转换为整数
          blog["title"] = row[1]; 
          blog["tag_id"] = atoi(row[2]); 
          blog["create_time"] = row[3];
          blogs->append(blog);
        }
        //遍历结果集合一定要记得释放
        mysql_free_result(result);
        return true;
      }
      
      //查找具体某一篇博客
      //blog同样为输出型参数，根据当前的blog_id再数据库中找到具体的
      //博客  通过blog参数返回给调用者
      bool SelectOne(int32_t blog_id, Json::Value* blog)
      {
        char sql[1024 * 4] = {0};
        sprintf(sql, "select * from blog_table where blog_id = %d", blog_id);
        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行sql失败！ %s\n", mysql_error(mysql_));
          return false;
        }
        MYSQL_RES* result = mysql_store_result(mysql_);
        if(result == NULL)
        {
          printf("获取结果失败！ %s\n", mysql_error(mysql_));
          return false;
        }

        int rows = mysql_num_rows(result);
        if(rows != 1)
        {
          printf("查询结果不为一条 %d\n", rows);
          return false;
        }
        
       MYSQL_ROW row = mysql_fetch_row(result); 
       (*blog)["blog_id"] = atoi(row[0]); 
       (*blog)["title"] = row[1]; 
       (*blog)["content"] = row[2]; 
       (*blog)["tag_id"] = atoi(row[3]); 
       (*blog)["creat_time"] = row[4];
        return true;
      }


      bool Update(const Json::Value& blog)
      {
        const std::string& content = blog["content"].asString();
        std::unique_ptr<char> content_escape(new char[content.size() * 2 + 1]);
        mysql_real_escape_string(mysql_, content_escape.get(), content.c_str(), content.size());

        std::unique_ptr<char> sql(new char[content.size() * 2 + 4096]);
        
        sprintf(sql.get(), "update blog_table set title='%s', tag_id=%d where blog_id=%d",
            blog["title"].asCString(), 
            content_escape.get(), 
            blog["tag_id"].asInt(), 
            blog["blog_id"].asInt());        
        
        int ret = mysql_query(mysql_, sql.get());
        if(ret != 0)
        {
          printf("执行sql失败！ sql=%s   %s\n", sql.get(), mysql_error(mysql_));
          return false;
        }
        return true;
      }


      bool Delete(int blog_id)
      {
        char sql[1024 * 4] = {0};
        sprintf(sql, "delete from blog_table where blog_id=%d", blog_id);
        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行sql失败！sql=%s, %s", sql  , mysql_error(mysql_));
          return false;
        }
        return true;
      }

    private:
      MYSQL* mysql_;
  };



  class TagTable
  {
    public:
      TagTable(MYSQL* mysql)
        :mysql_(mysql)
      {

      }

      bool Insert(const Json::Value& tag)
      {
        char sql[1024 * 4] = {0};
        sprintf(sql, "insert into tag_table value(null, '%s')", tag["tag_name"].asCString());
        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行sql失败！sql=%s  %s\n", sql, mysql_error(mysql_));
          return false;
        }
        return true;
      }



      bool Delete(int32_t tag_id)
      {
        char sql[1024 * 4] = {0};
        sprintf(sql, "delete from tag_table where tag_id=%d", tag_id);
        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行sql失败！ %s\n", mysql_error(mysql_));
          return false;
        }
        return true;
      }

      bool SelectAll(Json::Value* tags)
      {
        char sql[1024 * 4] = {0};
        sprintf(sql, "select * from tag_table");
        int ret = mysql_query(mysql_, sql);
        if(ret != 0)
        {
          printf("执行sql失败！ %s\n", mysql_error(mysql_));
          return false;
        }

        MYSQL_RES* result = mysql_store_result(mysql_);
        if(result == NULL)
        {
          printf("获取结果失败！%s\n",mysql_error(mysql_));
          return false;
        }
        int rows = mysql_num_rows(result);
        for(int i = 0; i < rows; i++)
        {
          MYSQL_ROW row = mysql_fetch_row(result);
          Json::Value tag;
          tag["tag_id"] = atoi(row[0]);
          tag["tag_name"] = row[1];
          tags->append(tag);
        }
        return true;
      }   
    private:
      MYSQL* mysql_;
  };

}//命名空间结束
