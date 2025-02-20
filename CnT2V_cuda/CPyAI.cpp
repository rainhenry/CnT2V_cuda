/*************************************************************

    程序名称:基于Python3原生C接口的AI C++类(阻塞)
    程序版本:REV 0.1
    创建日期:20240306
    设计编写:王祥福
    作者邮箱:rainhenry@savelife-tech.com

    版本修订
        REV 0.1   20240306      王祥福    创建文档

*************************************************************/

//  包含头文件
#include <stdio.h>
#include <stdlib.h>

#include "CPyAI.h"

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/ndarrayobject.h>
#include <numpy/npy_3kcompat.h>

//  初始化全局变量
bool CPyAI::Py_Initialize_flag = false;

//  构造函数
CPyAI::CPyAI()
{
    //  当没有初始化过
    if(!Py_Initialize_flag)
    {
        if(!Py_IsInitialized())
        {
            Py_Initialize();
            import_array_init();
        }
        Py_Initialize_flag = true;   //  标记已经初始化
    }

    //  控制权状态
    py_gil_st                   = -1;   

    //  python相关的私有数据
    py_cnt2v_module             = nullptr;

    //  翻译相关私有数据
    py_tsl_model_init           = nullptr;
    py_tsl_model_handle         = nullptr;       
    py_tsl_ex                   = nullptr;                

    //  文字生成视频相关私有数据
    py_ttv_model_pipe           = nullptr;
    py_ttv_model_pipe_handle    = nullptr;
    py_ttv_pipe_init            = nullptr;
    py_ttv_pipe_handle          = nullptr;
    py_ttv_ex                   = nullptr;
}

//  析构函数
CPyAI::~CPyAI()
{
    //  此处不可以调用Release，因为Python环境实际运行所在线程
    //  不一定和构造该类对象是同一个线程
}

//  释放资源
//  注意！该释放必须和执行本体在同一线程中！
void CPyAI::Release(void)
{

    if(py_ttv_ex != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_ex));
    }
    if(py_ttv_pipe_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_pipe_handle));
    }
    if(py_ttv_pipe_init != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_pipe_init));
    }
    if(py_ttv_model_pipe_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_model_pipe_handle));
    }
    if(py_ttv_model_pipe != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_model_pipe));
    }


    if(py_tsl_ex != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_ex));
    }
    if(py_tsl_model_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_model_handle));
    }
    if(py_tsl_model_init != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_model_init));
    }

    if(py_cnt2v_module != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_cnt2v_module));
    }

    if(py_gil_st != -1)
    {
        PyGILState_Release(static_cast<PyGILState_STATE>(py_gil_st));
    }

    if(Py_Initialize_flag)
    {
        //  程序退出时，由操作系统自动释放
        //Py_Finalize();
        //Py_Initialize_flag = false;   //  标记未初始化
    }
}

//  为了兼容Python C的原生API，独立封装numpy的C初始化接口
int CPyAI::import_array_init(void)
{
    import_array()
    return 0;
}

//  初始化
//  注意！该初始化必须和执行本体在同一线程中！
void CPyAI::Init(void)
{
    //  开启Python线程支持
    PyEval_InitThreads();
    PyEval_SaveThread();

    //  检测当前线程是否拥有GIL
    int ck = PyGILState_Check() ;
    if (!ck)
    {
        PyGILState_STATE state = PyGILState_Ensure(); //  如果没有GIL，则申请获取GIL
        py_gil_st = state;       //  定义于 /usr/include/python3.10/pystate.h 文件 94行，为枚举类型，可用int类型转存
    }

    //  构造基本Python环境
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('../CnT2V_cuda')");

    //  载入CnT2V_intel.py文件
    py_cnt2v_module = static_cast<void*>(PyImport_ImportModule("CnT2V_cuda"));

    //  检查是否成功
    if(py_cnt2v_module == nullptr)
    {
        printf("[Error] py_cnt2v_module == null!!\n");
        return;
    }

    //  初始化翻译模型
    py_tsl_model_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "translate_model_init"));
    Py_IncRef(static_cast<PyObject*>(py_tsl_model_init));
    PyObject* py_tsl_args = PyTuple_New(1);
    PyTuple_SetItem(py_tsl_args, 0, Py_BuildValue("s", CPYAI_TSL_MODEL_PATH));
    py_tsl_model_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_tsl_model_init), py_tsl_args));

    if(py_tsl_model_handle == nullptr)
    {
        printf("[Error] py_tsl_model_handle == null!!\n");
        return;
    }
    Py_IncRef(static_cast<PyObject*>(py_tsl_model_handle));

    //  载入执行翻译本体
    py_tsl_ex = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "translate_cn_to_en"));
    Py_IncRef(static_cast<PyObject*>(py_tsl_ex));

    //  获取原始模型管道
    py_ttv_model_pipe   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "get_origin_model_pipe"));
    Py_IncRef(static_cast<PyObject*>(py_ttv_model_pipe));
    PyObject* py_ttv_args = PyTuple_New(1);
    PyTuple_SetItem(py_ttv_args, 0, Py_BuildValue("s", CPYAI_TTV_MODEL_PATH));
    py_ttv_model_pipe_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_ttv_model_pipe), py_ttv_args));

    if(py_ttv_model_pipe_handle == nullptr)
    {
        printf("[Error] py_ttv_model_pipe_handle == null!!\n");
        return;
    }
    Py_IncRef(static_cast<PyObject*>(py_ttv_model_pipe_handle));

    //  初始化无加速器的管道
#if CPYAI_USE_GPU_ACC
    py_ttv_pipe_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "gpu_model_pipeline"));
#else  //  CPYAI_USE_GPU_ACC
    py_ttv_pipe_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "noacc_model_pipeline"));
#endif  //  CPYAI_USE_GPU_ACC
    Py_IncRef(static_cast<PyObject*>(py_ttv_pipe_init));
    py_ttv_args = PyTuple_New(1);
    PyTuple_SetItem(py_ttv_args, 0, static_cast<PyObject*>(py_ttv_model_pipe_handle));
    py_ttv_pipe_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_ttv_pipe_init), py_ttv_args));

    if(py_ttv_pipe_handle == nullptr)
    {
        printf("[Error] py_ttv_noacc_pipe_handle == null!!\n");
        return;
    }
    Py_IncRef(static_cast<PyObject*>(py_ttv_pipe_handle));

    //  载入无加速器的推理函数
    py_ttv_ex = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_cnt2v_module), "text_to_video"));
    Py_IncRef(static_cast<PyObject*>(py_ttv_ex));
}

//  执行中文到英文的翻译
std::string CPyAI::Translate_Cn2En_Ex(const char* prompt)
{
    //  返回字符串
    std::string re_str;

    //  构造输入数据
    PyObject* py_args = PyTuple_New(2);

    //  第一个参数为关键字
    PyObject* py_prompt = Py_BuildValue("s", prompt);
    PyTuple_SetItem(py_args, 0, py_prompt);

    //  第二个参数为模型句柄
    PyTuple_SetItem(py_args, 1, static_cast<PyObject*>(py_tsl_model_handle));

    //  执行
    PyObject* py_ret = PyObject_CallObject(static_cast<PyObject*>(py_tsl_ex), py_args);

    //  检查
    if(py_ret == nullptr)
    {
        printf("py_ret == nullptr\n");
        return re_str;
    }

    //  拿到返回字符串
    const char* tmp_str = PyUnicode_AsUTF8(py_ret);

    //  检查字符串
    if(tmp_str == nullptr)
    {
        printf("tmp_str == nullptr\n");
        return re_str;
    }

    //  赋值
    re_str = tmp_str;

    //  释放资源
    Py_DecRef(py_ret);
    Py_DecRef(py_prompt);
    //Py_DecRef(py_args);    //  由于其中包含了模型句柄,所以不能释放

    //  操作完成
    return re_str;
}

//  步骤调试
extern void debug_steps_print(int steps);

//  全局变量，用于步骤更新
extern int g_steps;

//  静态回调函数
static PyObject* callback(PyObject* self, PyObject* args)
{
    //  定义变量
    PyObject *p2;
    PyObject *p3;
    long i=0L;
    int tmp = 0;

    //  从参数列表中解析出输入值
    PyArg_ParseTuple(args, "lOO", &i, &p2, &p3);

    //  得到进度值
    tmp = static_cast<int>(i+1);
    g_steps = tmp;
    debug_steps_print(tmp);

    //  返回
    Py_RETURN_NONE;
}

//  执行英文文本到视频文件的生成
void CPyAI::Text_To_Video(
    const char* prompt,                  //  输入的英文文本
    int steps,                           //  推理步数
    CPyAI::SInferenceArgs args,          //  推理参数
    const char* out_gif_file,            //  输出的gif动图文件
    const char* out_mp4_file             //  输出的mp4视频文件
    )
{
    //  构造输入数据
    PyObject* py_args = PyTuple_New(9);

    //  第一个参数为关键字
    PyObject* py_prompt = Py_BuildValue("s", prompt);
    PyTuple_SetItem(py_args, 0, py_prompt);

    //  第二个参数为IR模型句柄
    PyTuple_SetItem(py_args, 1, static_cast<PyObject*>(py_ttv_pipe_handle));

    //  第三个参数为推理步数
    PyObject* py_steps = Py_BuildValue("i", steps);
    PyTuple_SetItem(py_args, 2, py_steps);

    //  第四个参数为视频宽度
    PyObject* py_width = Py_BuildValue("i", args.width);
    PyTuple_SetItem(py_args, 3, py_width);

    //  第五个参数为视频高度
    PyObject* py_height = Py_BuildValue("i", args.height);
    PyTuple_SetItem(py_args, 4, py_height);

    //  第六个参数为总帧数
    PyObject* py_frames = Py_BuildValue("i", args.total_frames);
    PyTuple_SetItem(py_args, 5, py_frames);

    //  第七个参数为输出gif动图文件
    PyObject* py_output_gif_file = Py_BuildValue("s", out_gif_file);
    PyTuple_SetItem(py_args, 6, py_output_gif_file);

    //  第八个参数为输出mp4视频文件
    PyObject* py_output_mp4_file = Py_BuildValue("s", out_mp4_file);
    PyTuple_SetItem(py_args, 7, py_output_mp4_file);

    //  第九个参数为进度回调函数
    PyMethodDef CFunc = {"callback", callback, METH_VARARGS, ""};
    PyObject* pCallbackFunc = PyCFunction_New(&CFunc, nullptr);
    PyObject* py_progress = Py_BuildValue("O", pCallbackFunc);
    PyTuple_SetItem(py_args, 8, py_progress);

    //  执行
    PyObject_CallObject(static_cast<PyObject*>(py_ttv_ex), py_args);

    //  释放资源
    Py_DecRef(py_progress);
    Py_DecRef(py_output_mp4_file);
    Py_DecRef(py_output_gif_file);
    Py_DecRef(py_frames);
    Py_DecRef(py_height);
    Py_DecRef(py_width);
    Py_DecRef(py_steps);
    Py_DecRef(py_prompt);
    //Py_DecRef(py_args);    //  由于其中包含了模型句柄,所以不能释放

    //  操作完成
    return;
}
