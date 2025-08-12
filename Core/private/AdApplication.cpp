#include "AdApplication.h"
#include "AdLog.h"
#include "Render/AdRenderContext.h"
#include "ECS/AdEntity.h"

namespace ade {
	AdAppContext AdApplication::sAppContext{};

	/**
	 * @brief ����Ӧ�ó��򣬳�ʼ����־�����ڡ���Ⱦ�����ĵȺ������
	 * @param argc �����в�������
	 * @param argv �����в�������
	 */
	void AdApplication::Start(int argc, char** argv) {
		Adlog::Init(); // ��ʼ����־ϵͳ

		ParseArgs(argc, argv); // ���������в���
		OnConfiguration(&mAppSettings); // ����Ӧ�ó�������

		// �������ں���Ⱦ������
		mWindow = AdWindow::Create(mAppSettings.width, mAppSettings.height, mAppSettings.title);
		mRenderContext = std::make_shared<AdRenderContext>(mWindow.get());

		// ����ȫ��Ӧ��������
		sAppContext.app = this;
		sAppContext.renderCxt = mRenderContext.get();

		OnInit(); // ���ó�ʼ���ص�
		LoadScene(); // ���س���

		mStartTimePoint = std::chrono::steady_clock::now(); // ��¼����ʱ���
	}

	/**
	 * @brief ֹͣӦ�ó���ж�س�����ִ�������߼�
	 */
	void AdApplication::Stop() {
		UnLoadScene(); // ж�ص�ǰ����
		OnDestroy(); // ִ�����ٻص�
	}

	/**
	 * @brief Ӧ�ó�����ѭ���������¼��������߼�����Ⱦ
	 */
	void AdApplication::MainLoop() {
		mLastTimePoint = std::chrono::steady_clock::now(); // ��ʼ����һ֡ʱ���
		while (!mWindow->ShouldClose()) { // ��ѭ��ֱ�����ڹر�
			mWindow->PollEvents(); // �������¼�

			// ����֡���ʱ��
			float deltaTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - mLastTimePoint).count();
			mLastTimePoint = std::chrono::steady_clock::now();
			mFrameIndex++; // ֡����������

			if (!bPause) { // ���δ��ͣ��������߼�
				OnUpdate(deltaTime);
			}
			OnRender(); // ִ����Ⱦ�߼�

			mWindow->SwapBuffer(); // ������������ʾ����
		}
	}

	/**
	 * @brief ���������в�����ĿǰΪ��ʵ�֣�
	 * @param argc �����в�������
	 * @param argv �����в�������
	 */
	void AdApplication::ParseArgs(int argc, char** argv) {

	}

	/**
	 * @brief ����ָ��·���ĳ����ļ�
	 * @param filePath �����ļ�·������ǰδʹ�ã�
	 * @return �Ƿ���سɹ�
	 */
	bool AdApplication::LoadScene(const std::string& filePath) {
		if (mScene) { // ������г�������ж��
			UnLoadScene();
		}
		mScene = std::make_unique<AdScene>(); // �����³���
		OnSceneInit(mScene.get()); // ��ʼ�������ص�
		sAppContext.scene = mScene.get(); // ����ȫ���������еĳ���ָ��
		return true;
	}

	/**
	 * @brief ж�ص�ǰ���صĳ���
	 */
	void AdApplication::UnLoadScene() {
		if (mScene) { // ������ڳ���
			OnSceneDestroy(mScene.get()); // ִ�г������ٻص�
			mScene.reset(); // �ͷų�����Դ
			sAppContext.scene = nullptr; // ���ȫ���������еĳ���ָ��
		}
	}
}