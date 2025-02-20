/*************************************************************

    程序名称:基于Python3原生C接口的AI C++类(阻塞)
    程序版本:REV 0.1
    创建日期:20240306
    设计编写:王祥福
    作者邮箱:rainhenry@savelife-tech.com

    版本修订
        REV 0.1   20240306      王祥福    创建文档

*************************************************************/
//------------------------------------------------------------
//  重定义保护
#ifndef __CPYAI_H__
#define __CPYAI_H__

//------------------------------------------------------------
//  包含头文件
#include <string>
#include <list>

//------------------------------------------------------------
//  宏定义

//  翻译模型路径
#define CPYAI_TSL_MODEL_PATH              "../opus-mt-zh-en"

//  文本生成视频模型路径
#define CPYAI_TTV_MODEL_PATH              "../zeroscope_v2_576w"

//  用于保存模型信息的文件名
#define CPYAI_MODEL_CONF_FILE             "model.cfg"

//  是否使用GPU加速
#define CPYAI_USE_GPU_ACC                 1

//------------------------------------------------------------
//  类定义
class CPyAI
{
public:
    //  推理参数结构体
    typedef struct
    {
        bool args_valid;                     //  参数是否有效
        int  width;                          //  视频宽度
        int  height;                         //  视频高度
        int  total_frames;                   //  视频总帧数
    }SInferenceArgs;

public:
    //  构造与析构函数
    CPyAI();
    ~CPyAI();

    //  释放资源
    //  注意！该释放必须和执行本体在同一线程中！
    void Release(void);

    //  初始化
    //  注意！该初始化必须和执行本体在同一线程中！
    void Init(void);

    //  执行中文到英文的翻译
    std::string Translate_Cn2En_Ex(const char* prompt);

    //  执行英文文本到视频文件的生成
    void Text_To_Video(
        const char* prompt,                  //  输入的英文文本
        int steps,                           //  推理步数
        SInferenceArgs args,                 //  推理参数
        const char* out_gif_file,            //  输出的gif动图文件
        const char* out_mp4_file             //  输出的mp4视频文件
        );

private:
    //  为了兼容Python C的原生API，独立封装numpy的C初始化接口
    int import_array_init(void);

    //  静态python环境初始化标志
    static bool Py_Initialize_flag;

    //  控制权状态
    int py_gil_st;    

    //  python相关的私有数据
    void* py_cnt2v_module;               //  模块

    //  翻译相关私有数据
    void* py_tsl_model_init;             //  模型初始化
    void* py_tsl_model_handle;           //  模型句柄
    void* py_tsl_ex;                     //  执行一次翻译

    //  文字生成视频相关私有数据
    void* py_ttv_model_pipe;             //  获取原始模型管道
    void* py_ttv_model_pipe_handle;      //  原始模型管道句柄
    void* py_ttv_pipe_init;              //  管道初始化
    void* py_ttv_pipe_handle;            //  管道句柄
    void* py_ttv_ex;                     //  执行一次文字生成视频

};


//------------------------------------------------------------
#endif  //  __CPYAI_H__


