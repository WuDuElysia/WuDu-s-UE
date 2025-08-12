# ����Vulkan��ECS�ܹ�3D��Ⱦ����

## ��Ŀ����

����Ŀ��һ������Vulkanͼ��API���ִ���3D��Ⱦ���棬����Entity-Component-System (ECS)�ܹ����ģʽ����Ŀʵ������������Ⱦ���ߣ�֧��3Dģ����Ⱦ������ϵͳ��������ͼ�ȹ��ܣ���ͨ��ECS�ܹ��ṩ�����ĳ�������������

## ��Ŀ�ṹ
```mermaid
graph TD
    A[��Ŀ��Ŀ¼] 
    A --> B(Core ��������ģ��)
    B --> B1(public ����ͷ�ļ�)
    B1 --> B1a(ECS)
    B1a --> B1a1(Component �������)
    B1a --> B1a2(System ϵͳʵ��)
    B1a --> B1a3(AdEntity.h ʵ����)
    B1a --> B1a4(AdScene.h ��������)
    B1 --> B1b(Graphic Vulkan��װ��)
    B1b --> B1b1(AdVKDevice.h �豸����)
    B1b --> B1b2(AdVKRenderPass.h ��Ⱦͨ��)
    B1b --> B1b3(AdVKPipeline.h ͼ�ι���)
    B1b --> B1b4(AdVKImage.h ͼ����Դ)
    B1b --> B1b5(AdVKBuffer.h ��������Դ)
    B1b --> B1b6(... ����Vulkan����)
    B1 --> B1c(Render ��Ⱦ�����)
    B1c --> B1c1(AdRenderTarget.h ��ȾĿ��)
    B1c --> B1c2(AdRenderer.h ��Ⱦ��)
    B1c --> B1c3(AdMesh.h ��������)
    B1c --> B1c4(AdTexture.h ������Դ)
    B1 --> B1d(... ��������ģ��)
    B --> B2(private ʵ���ļ�)
    B2 --> B2a(... ��Ӧpublic��ʵ��)
    A --> C(Platform ƽ̨��ش���)
    C --> C1(... ����ϵͳ������ϵͳ��)
    A --> D(Sample ʾ��Ӧ��)
    D --> D1(04_ECS_Entity ECSʵ����Ⱦʾ��)
    D1 --> D1a(Main.cpp ��Ӧ�����)
    A --> E(ThirdParty ��������)
    E --> E1(EnTT ECS���)
    E --> E2(GLM ��ѧ��)
    E --> E3(Vulkan Vulkan SDK)
    E --> E4(stb ͼ����ؿ�)
```

