"""Create/clean an isolated organization for optional native account integration tests."""
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(sys.argv[2]).resolve() / 'label_studio'))
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'core.settings.label_studio')
os.environ.setdefault('SENTRY_DSN', '')
import django
django.setup()
from django.db import transaction
from users.models import User
from organizations.models import Organization
from projects.models import Project

email = os.environ['QML_ACCOUNT_LIVE_EMAIL']
assert email.startswith('qml-account-qa-') and email.endswith('@example.invalid')
other_email = email.replace('@', '-member@')
signup_email = email.replace('@', '-signup@')
if sys.argv[1] == 'setup':
    with transaction.atomic():
        user = User.objects.create_user(email=email, password=os.environ['QML_ACCOUNT_LIVE_PASSWORD'])
        org = Organization.create_organization(created_by=user, title='QML account isolated QA')
        user.active_organization = org
        user.save(update_fields=['active_organization'])
        member = User.objects.create_user(email=other_email, password=os.environ['QML_ACCOUNT_LIVE_PASSWORD'])
        org.add_user(member)
        member.active_organization = org
        member.save(update_fields=['active_organization'])
        if os.environ.get('QML_PROJECT_MANAGEMENT') == '1':
            from data_manager.models import View
            project = Project.objects.create(organization=org, created_by=user, title='QML management QA fixture')
            View.objects.create(project=project, user=user, data={'title': 'QA first tab'})
            View.objects.create(project=project, user=user, data={'title': 'QA second tab'})
        if os.environ.get('QML_EDITOR_DATA') == '1':
            from django.core.files.base import ContentFile
            from data_import.models import FileUpload
            from tasks.models import Task, Annotation, Prediction
            cfg = '<View><Image name="image" value="$image"/><RectangleLabels name="bbox" toName="image"><Label value="Car"/></RectangleLabels></View>'
            project = Project.objects.create(organization=org, created_by=user, title='QML management QA data manager', label_config=cfg, model_version='qa-v1')
            image_bytes = (Path(__file__).resolve().parent.parent / 'assets' / 'bounding-boxes.png').read_bytes()
            upload = FileUpload.objects.create(project=project, user=user, file=ContentFile(image_bytes, name='dm-qa.png'))
            tasks = [Task.objects.create(project=project, data={'image':upload.url, 'text':f'item-{i:02}', 'group':'A' if i%2==0 else 'B', 'rank':i}) for i in range(64)]
            Task.objects.create(project=project, data=dict(tasks[0].data))
            result = [{'id':'qa-rectangle', 'from_name':'bbox', 'to_name':'image', 'type':'rectanglelabels', 'original_width':400, 'original_height':300, 'image_rotation':0, 'value':{'x':10,'y':10,'width':20,'height':25,'rotation':0,'rectanglelabels':['Car']}}]
            for task in tasks[:2]:
                Annotation.objects.create(project=project, task=task, completed_by=member, result=result)
            Annotation.objects.create(project=project, task=tasks[0], completed_by=user, result=result)
            for task in tasks[:3]:
                Prediction.objects.create(task=task, result=result, score=0.8, model_version='qa-v1')
            project.summary.update_data_columns(list(project.tasks.all()))
            project.update_tasks_counters_and_is_labeled(tasks_queryset=project.tasks.all(), run_sync=True)
        print('Isolated account fixture ready.')
elif sys.argv[1] == 'verify-projects':
    user = User.objects.get(email=email)
    project = Project.objects.get(organization=user.active_organization, title='QML management QA fixture')
    assert project.views.count() == 0
    assert project.tasks.count() == 3
    assert project.annotations.count() > 0
    assert project.sampling == 'Uniform sampling'
    assert project.color.lower() == '#ff8844'
    print('Verified project tabs deleted; tasks, annotations, color and sampling preserved.')
elif sys.argv[1] == 'verify-editor-data':
    from tasks.models import Annotation, Prediction
    user = User.objects.get(email=email)
    member = User.objects.get(email=other_email)
    project = Project.objects.get(organization=user.active_organization, title='QML management QA data manager')
    assert project.tasks.count() == 61, project.tasks.count()
    assert not Annotation.objects.filter(project=project, completed_by=member).exists()
    assert Annotation.objects.filter(project=project, completed_by=user).exists()
    assert not Prediction.objects.filter(task__project=project).exists()
    assert project.views.count() == 1
    assert project.views.first().data['title'] == 'Renamed QA view'
    print('Verified filters, saved tab, actor-specific annotation deletion, predictions conversion/deletion, deduplication, selected deletion and task preservation.')
elif sys.argv[1] == 'cleanup':
    user = User.objects.filter(email=email).first()
    if user:
        for org in Organization.objects.filter(created_by=user):
            assert org.title.startswith('QML account')
            for project in Project.objects.filter(organization=org):
                assert project.title.startswith('QML management QA')
                from core.utils.common import temporary_disconnect_all_signals
                with temporary_disconnect_all_signals():
                    project.delete()
            org.jwt.delete()
            org.session_timeout_policy.delete()
            org.delete()
    User.objects.filter(email__in=[email, other_email, signup_email]).delete()
    print('Isolated account fixture cleaned.')
