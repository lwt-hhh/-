#include<signal.h>
#include "httplib.h"
#include "db.hpp"

MYSQL* mysql = NULL;

int main()
{
  using namespace httplib;
  using namespace Blog_System;
  
  //先和数据库建立好连接
  mysql = mysqlinit(); 
  signal(SIGINT, [](int) {  mysqlrelase(mysql);  exit(0); }); 

  //创建相应对象
  BlogTable blog_table(mysql); 
  TagTable tag_table(mysql);

  //创建服务器
   Server server; 
  //设置路由，以下的路由是指  把方法 和 path 映射到哪个处理函数，表示关联关系
  

  //新增博客
  server.Post("/blog", [&blog_table](const Request& req, Response& resp)
      { 
        Json::FastWriter writer;
        Json::Reader reader;
        Json::Value req_json;
        Json::Value resp_json;
        //获取body 并解析成Json;
        bool ret = reader.parse(req.body, req_json);
        if(!ret)
        {
          //解析出错，给用户提示
          printf("解析请求失败！ %s\n", req.body.c_str());
          //构造一个Json响应对象，告诉客户端出错了
          resp_json["ok"] = false;
          resp_json["reason"] = "parse Request failed\n";
          resp.status = 400;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }

        //进行参数校验
        if(req_json["title"].empty() || req_json["content"].empty() || req_json["tag_id"].empty() || req_json["create_time"].empty())
        {
          resp_json["ok"] = false; 
          resp_json["reason"] = "Request fields error!\n"; 
          resp.status = 400; 
          resp.set_content(writer.write(resp_json), "application/json"); //application/json
                                                                         //代表返回的是Json格式
          return;
        }
        //调用数据库接口进行 数据操作
        ret = blog_table.Insert(req_json);
        if(!ret)
        {
          resp_json["ok"] = false; 
          resp_json["reason"] = "Insert failed!\n"; 
          resp.status = 500; 
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }

        //封装正确的返回结果
        resp_json["ok"] = true; 
        resp.set_content(writer.write(resp_json), "application/json"); 
        return;
      });

  //查看所有博客列表
  server.Get("/blog", [&blog_table](const Request& req, Response& resp)
      {
        Json::Value resp_json;
        Json::FastWriter writer;
        Json::Reader reader;
        const std::string& tag_id = req.get_param_value("tag_id");

        Json::Value blogs;
        bool ret = blog_table.SelectAll(&blogs, tag_id);
        if(!ret)
        {
          resp_json["ok"] = false; 
          resp_json["reason"] = "SelectAll failed!\n"; 
          resp.status = 500; 
          resp.set_content(writer.write(resp_json), "application/json"); 
          return;
        }

        //构造响应结果
        resp.set_content(writer.write(blogs), "application/json"); 
        return;
      });

  //查看某个博客
  server.Get(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp)
      {
        Json::Value resp_json;
        Json::FastWriter writer;
        //解析获取blog_id
        int blog_id = std::stoi(req.matches[1]);

        //调用数据库接口 查看指定博客
        bool ret = blog_table.SelectOne(blog_id, &resp_json);
        if(!ret)
        {
          resp_json["ok"] = false;
          resp_json["reason"] = "SelectOne failed!";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }

        //包装正确响应
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      });

  //修改某个博客
  server.Put(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp)
        {
          Json::Value req_json;
          Json::Value resp_json;
          Json::FastWriter writer;
          Json::Reader reader;
          //获取到blog_id
          int blog_id = std::stoi(req.matches[1]);
          //解析博客信息
          bool ret = reader.parse(req.body, req_json);
          if(!ret)
          {
            resp_json["ok"] = false;
            resp_json["reason"] = "parse failed!";
            resp.status = 400;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }
          req_json["blog_id"] = blog_id;
          //校验博客信息
          if(req_json["title"].empty() || req_json["content"].empty() 
              || req_json["tag_id"].empty())
          {
            resp_json["ok"] = false; 
            resp_json["reason"] = "Request has no name or price!\n"; 
            resp.status = 400; 
            resp.set_content(writer.write(resp_json), "application/json"); 
            return;
          }
          //调用数据库接口进行修改
          ret = blog_table.Update(req_json);
          if(!ret)
          {
            resp_json["ok"] = false; 
            resp_json["reason"] = "Update failed!\n"; 
            resp.status = 500; 
            resp.set_content(writer.write(resp_json), "application/json"); 
            return;
          }
          //封装正确的数据
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        });


  //删除博客
  server.Delete(R"(/blog/(\d+))", [&blog_table](const Request& req, Response& resp)
        {
          Json::Value resp_json;
          Json::FastWriter writer;
          //解析获取blog_id
          int blog_id = std::stoi(req.matches[1]);
          //调用数据库接口删除博客
          bool ret = blog_table.Delete(blog_id);
          if(!ret)
          {
            resp_json["ok"] = false;
            resp_json["reason"] = "Delete failed!";
            resp.status = 500;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }
          //封装正确的响应
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        });


  //新增标签
  server.Post("/tag", [&tag_table](const Request& req, Response& resp)
        {
          Json::Value req_json;
          Json::Value resp_json;
          Json::FastWriter writer;
          Json::Reader reader;
          //请求解析成Json格式
          bool ret = reader.parse(req.body, req_json);
          if(!ret)
          {
            resp_json["ok"] = false;
            resp_json["reason"] = "parse failed!";
            resp.status = 400;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }
          //校验标签格式
          if(req_json["tag_name"].empty())
          {
            resp_json["ok"] = false;
            resp_json["reason"] = "数据格式有误！";
            resp.status = 400;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }
          //调用数据库接口，插入标签
          ret = tag_table.Insert(req_json);
          if(!ret)
          {
            resp_json["ok"] = false;
            resp_json["reason"] = "插入失败！";
            resp.status = 500;
            resp.set_content(writer.write(resp_json), "application/json");
            return;
          }

          //构造正确的响应
          resp_json["ok"] = true;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        });


  //删除标签
  server.Delete(R"(/tag/(\d+))", [&tag_table](const Request& req, Response& resp)
      {
        Json::Value resp_json;
        Json::FastWriter writer;
        //解析出tag_id
        int tag_id = std::stoi(req.matches[1]);
        //执行数据库操作删除标签
        bool ret = tag_table.Delete(tag_id);
        if(!ret)
        {
          resp_json["ok"] = false; 
          resp_json["reason"] = "TagTable Insert failed!\n"; 
          resp.status = 500; 
          resp.set_content(writer.write(resp_json), "application/json"); 
          return;
        }

        //包装正确的响应
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      });


  //查看所有标签  
  server.Get("/tag", [&tag_table](const Request& req, Response& resp)
      {
        (void) req;
        Json::Value resp_json;
        Json::FastWriter writer;
        Json::Reader reader;
        //调用数据库接口查询数据
        Json::Value tags;
        bool ret = tag_table.SelectAll(&tags);
        if(!ret)
        {
          resp_json["ok"] = false;
          resp_json["reason"] = "查找所有博客失败！";
          resp.status = 500;
          resp.set_content(writer.write(resp_json), "application/json");
          return;
        }
        //构造正确的响应结果
        resp_json["ok"] = true;
        resp.set_content(writer.write(resp_json), "application/json");
        return;
      });
  server.set_base_dir("./wwwroot");
  server.listen("0.0.0.0", 9093);

  return 0;
}

